/* HPUX_ID: @(#) $Revision: 70.2 $ */
/*

 *      Copyright (c) 1984, 1985, 1986, 1987, 
 *                  1988, 1989   AT&T
 *      All Rights Reserved

 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
 *      CODE OF AT&T.
 *      The copyright notice above does not 
 *      evidence any actual or intended
 *      publication of such source code.

 */
/*
 *	UNIX shell
 *	S. R. Bourne
 *	Rewritten by David Korn
 *
 *	AT&T Bell Laboratories
 *
 */


#include	<errno.h>
#include	"defs.h"
#include	"sym.h"
#include	"builtins.h"
#include	"test.h"
#include	"timeout.h"
#include	"history.h"


#ifdef MULTIBYTE
#include	"national.h"
    MSG e_version = "\n@(#)Version M-11/16/88\0\n";
#else
    MSG e_version = "\n@(#)Version 11/16/88\0\n";
#endif /* MULTIBYTE */

extern struct Bfunction sh_randnum;
extern struct Bfunction sh_seconds;
extern struct Bfunction line_numbers;

/* error messages */
MSG	e_timewarn	= "\r\n\007shell time out in 60 seconds";
MSG	e_timeout	= "timed out waiting for input";
MSG	e_mailmsg	= "you have mail in $_";
MSG	e_query		= "no query process";
MSG	e_history	= "no history file";
MSG	e_option	= "bad option(s)";
MSG	e_space		= "no space";
MSG	e_argexp	= "argument expected";
MSG	e_bracket	= "] missing";
MSG	e_number	= "bad number";
MSG	e_nullset	= "parameter null or not set";
MSG	e_notset	= "parameter not set";
MSG	e_subst		= "bad substitution";
MSG	e_create	= "cannot create";
MSG	e_restricted	= "restricted";
MSG	e_fork		= "cannot fork: too many processes";
MSG	e_pexists	= "process already exists";
MSG	e_fexists	= "file already exists";
MSG	e_swap		= "cannot fork: no swap space";
MSG	e_pipe		= "cannot make pipe";
MSG	e_open		= "cannot open";
MSG	e_logout	= "Use 'exit' to logout";
MSG	e_arglist	= "arg list too long";
MSG	e_txtbsy	= "text busy";
MSG	e_toobig	= "too big";
MSG	e_exec		= "cannot execute";
MSG	e_pwd		= "cannot access parent directories";
MSG	e_found		= " not found";
MSG	e_flimit	= "too many open files";
MSG	e_ulimit	= "exceeds allowable limit";
MSG	e_subscript	= "subscript out of range";
MSG	e_nargs		= "bad argument count";
#ifdef ELIBACC
    /* shared library error messages */
    MSG	e_libacc 	= "can't access a needed shared library";
    MSG	e_libbad	= "accessing a corrupted shared library";
    MSG	e_libscn	= ".lib section in a.out corrupted";
    MSG	e_libmax	= "attempting to link in too many libs";
#endif	/* ELIBACC */
#ifdef EMULTIHOP
    MSG   e_multihop	= "multihop attempted";
#endif /* EMULTIHOP */
#ifdef ENAMETOOLONG
    MSG   e_longname	= "name too long";
#endif /* ENAMETOOLONG */
#ifdef ENOLINK
    MSG	e_link		= "remote link inactive";
#endif /* ENOLINK */
MSG	e_access	= "permission denied";
MSG	e_direct	= "bad directory";
MSG	e_notdir	= "not a directory";
MSG	e_file		= "bad file unit number";
MSG	e_trap		= "bad trap";
MSG	e_readonly	= "is read only";
MSG	e_ident		= "is not an identifier";
MSG	e_aliname	= "invalid alias name";
MSG	e_testop	= "unknown test operator";
MSG	e_alias		= " alias not found";
MSG	e_function	= "unknown function";
MSG	e_format	= "bad format";
MSG	e_on		= "on";
MSG	e_off		= "off";
MSG	is_reserved	= " is a keyword";
MSG	is_builtin	= " is a shell builtin";
MSG	is_alias	= " is an alias for ";
MSG	is_function	= " is a function";
MSG	is_xalias	= " is an exported alias for ";
MSG	is_talias	= " is a tracked alias for ";
MSG	is_xfunction	= " is an exported function";
MSG	is_ufunction	= " is an undefined function";
MSG	is_		= " is ";
MSG	e_fnhdr		= "\n{\n";
MSG	e_runvi		= "fc -e \"${VISUAL:-${EDITOR:-vi}}\" ";
#ifdef JOBS
#   ifdef SIGTSTP
	MSG	e_newtty	= "Switching to new tty driver...";
	MSG	e_oldtty	= "Reverting to old tty driver...";
	MSG	e_no_start	= "Cannot start job control";
	MSG	e_no_jctl	= "No job control";
	MSG	e_terminate	= "You have stopped jobs";
#   endif /*SIGTSTP */
   MSG	e_Done		= " Done";
   MSG	e_nlspace	= "\n      ";
   MSG	e_Running	= " Running";
   MSG	e_ambiguous	= "Ambiguous";
   MSG	e_running	= "You have running jobs";
   MSG	e_no_job	= "no such job";
   MSG	e_no_proc	= "no such process";
   MSG	e_killcolon	= "kill: ";
   MSG	e_jobusage	= "Arguments must be %job or process ids";
   MSG	e_kill		= "kill";
#endif /* JOBS */
MSG	e_coredump	= "(coredump)";
#ifdef DEVFD
    MSG	e_devfd		= "/dev/fd/";
#endif /* DEVFD */
#ifdef VPIX
    MSG	e_vpix		= "/vpix";
    MSG	e_vpixdir	= "/usr/bin";
#endif /* VPIX */
MSG	e_wrongbinary	= "Executable file incompatible with hardware";
MSG	e_bad_signal	= "invalid signal";

/* string constants */
#ifdef POSIX3 		/* added option -e */
MSG	test_unops	= "eHLSVOGaohrwxdcbfugkpsnzt";
#else
MSG	test_unops	= "HLSVOGaohrwxdcbfugkpsnzt";
#endif /* POSIX3 */
MSG	e_heading	= "Current option settings";
MSG	e_nullstr	= "";
MSG	e_sptbnl	= " \t\n";
MSG	e_defpath	= "/bin:/usr/bin:";
MSG	e_defedit	= "/bin/ed";
MSG	e_colon		= ": ";
MSG	e_minus		= "-";
MSG	e_endoffile	= "end of file";
MSG	e_unexpected 	= " unexpected";
MSG	e_unmatched 	= " unmatched";
MSG	e_unknown 	= "<command unknown>";
MSG	e_atline	= " at line ";
MSG	e_devnull	= "/dev/null";
MSG	e_traceprompt	= "+ ";
MSG	e_supprompt	= "# ";
MSG	e_stdprompt	= "$ ";
MSG	e_profile	= "${HOME:-.}/.profile";
MSG	e_sysprofile	= "/etc/profile";
MSG	e_suidprofile	= "/etc/suid_profile";
MSG	e_crondir	= "/usr/spool/cron/atjobs";
#ifndef INT16
   MSG	e_prohibited	= "login setuid/setgid shells prohibited";
#endif /* INT16 */
#ifdef SUID_EXEC
   MSG	e_suidexec	= "/etc/suid_exec";
#endif /* SUID_EXEC */
MSG	e_devfdNN	= "/dev/fd/+([0-9])";
MSG	hist_fname	= "/.sh_history";
MSG	e_unlimited	= "unlimited";
#ifdef ECHO_N
   MSG	e_echobin	= "/bin/echo";
   MSG	e_echoflag	= "-R";
#endif	/* ECHO_N */
MSG	e_test		= "test";
MSG	e_let		= "let";
MSG	e_dot		= ".";
MSG	e_bltfn		= "function ";
MSG	e_intbase	= "base";
MSG	e_envmarker	= "A__z";
#ifdef FLOAT
    MSG	e_precision	= "precision";
#endif /* FLOAT */
#ifdef PDUBIN
        MSG	e_setpwd	= "PWD=`/usr/pdu/bin/pwd 2>/dev/null`";
#else
        MSG	e_setpwd	= "PWD=`/bin/pwd 2>/dev/null`";
#endif /* PDUBIN */
MSG	e_real		= "\nreal";
MSG	e_user		= "user";
MSG	e_sys		= "sys";

#ifdef apollo
#   undef NULL
#   define NULL ""
#   define e_nullstr	""
#endif	/* apollo */

/* built in names */
const struct name_value node_names[] =
{
	"PATH",		NULL,	0,
	"PS1",		NULL,	0,
	"PS2",		"> ",	N_FREE,
#ifdef apollo
	"IFS",		" \t\n",	N_FREE,
#else
	"IFS",		e_sptbnl,	N_FREE,
#endif	/* apollo */
	"PWD",		NULL,	0,
	"HOME",		NULL,	0,
	"MAIL",		NULL,	0,
	"REPLY",	NULL,	0,
	"SHELL",	"/bin/sh",	N_FREE,
	"EDITOR",	NULL,	0,
#ifdef apollo
	"MAILCHECK",	NULL,	N_FREE|N_INTGER,
	"RANDOM",	NULL,	N_FREE|N_INTGER,
#else
	"MAILCHECK",	(char*)(&sh_mailchk),	N_FREE|N_INTGER,
	"RANDOM",	(char*)(&sh_randnum),	N_FREE|N_INTGER|N_BLTNOD,
#endif	/* apollo */
	"ENV",		NULL,	0,
	"HISTFILE",	NULL,	0,
	"HISTSIZE",	NULL,	0,
	"FCEDIT",	"/bin/ed",	N_FREE,
	"CDPATH",	NULL,	0,
	"MAILPATH",	NULL,	0,
	"PS3",		"#? ",	N_FREE,
	"OLDPWD",	NULL,	0,
	"VISUAL",	NULL,	0,
	"COLUMNS",	NULL,	0,
	"LINES",	NULL,	0,
#ifdef apollo
	"PPID",		NULL,	N_FREE|N_INTGER,
#ifndef POSIX2
	"_",		NULL,	N_FREE|N_INDIRECT|N_EXPORT,
#else
	"_",		NULL,	N_FREE|N_INDIRECT,
#endif
	"TMOUT",	NULL,	N_INTGER,
	"SECONDS",	NULL,	N_FREE|N_INTGER|N_BLTNOD,
	"ERRNO",	NULL,	N_FREE|N_INTGER,
	"LINENO",	NULL,	N_FREE|N_INTGER|N_BLTNOD,
	"OPTIND",	NULL,	N_FREE|N_INTGER,
	"OPTARG",	NULL,	N_FREE|N_INDIRECT,
#else
	"PPID",		(char*)(&sh.ppid),	N_FREE|N_INTGER,
#ifndef POSIX2
	"_",		(char*)(&sh.lastarg),	N_FREE|N_INDIRECT|N_EXPORT,
#else
	"_",		(char*)(&sh.lastarg),	N_FREE|N_INDIRECT,
#endif
	"TMOUT",	(char*)(&sh_timeout),	N_INTGER,
	"SECONDS",	(char*)(&sh_seconds),	N_FREE|N_INTGER|N_BLTNOD,
	"ERRNO",	(char*)(&errno),	N_FREE|N_INTGER,
	"LINENO",	(char*)(&line_numbers),	N_FREE|N_INTGER|N_BLTNOD,
	"OPTIND",	(char*)(&opt_index),	N_FREE|N_INTGER,
	"OPTARG",	(char*)(&opt_arg),	N_FREE|N_INDIRECT,
#endif	/* apollo */
	"PS4",		NULL,	0,
	"FPATH",	NULL,	0,
	"LANG",		NULL,	0,
	"LC_CTYPE",	NULL,	0,
	"LC_COLLATE",	NULL,	0,
#ifdef VPIX
	"DOSPATH",	NULL,	0,
	"VPIXDIR",	NULL,	0,
#endif	/* VPIX */
#ifdef ACCT
	"SHACCT",	NULL,	0,
#endif	/* ACCT */
#ifdef MULTIBYTE
	"CSWIDTH",	NULL,	0,
#endif /* MULTIBYTE */
#ifdef POSIX2
	"OPTERR",	(char*)(&opt_err),	N_FREE|N_INTGER,
#endif /* POSIX2 */	   
	e_nullstr,	NULL,	0
};

#ifdef VPIX
   const char *suffix_list[] = { ".com", ".exe", ".bat", e_nullstr };
#endif	/* VPIX */

/* built in aliases - automatically exported */
const struct name_value alias_names[] =
{
	"autoload",	"typeset -fu",	N_FREE|N_EXPORT,
#ifdef POSIX2
   	"command",	"command ",	N_FREE|N_EXPORT,
#endif  /* POSIX2 */
	"false",	"let 0",	N_FREE|N_EXPORT,
	"functions",	"typeset -f",	N_FREE|N_EXPORT,
	"hash",		"alias -t -",	N_FREE|N_EXPORT,
	"history",	"fc -l",	N_FREE|N_EXPORT,
	"integer",	"typeset -i",	N_FREE|N_EXPORT,
	"nohup",	"nohup ",	N_FREE|N_EXPORT,
	"r",		"fc -e -",	N_FREE|N_EXPORT,
	"true",		":",		N_FREE|N_EXPORT,
	"type",		"whence -v",	N_FREE|N_EXPORT,
#  ifdef SIGTSTP
	"stop",		"kill -STOP",	N_FREE|N_EXPORT,
	"suspend",	"kill -STOP $$",	N_FREE|N_EXPORT,
#  endif /*SIGTSTP */
	e_nullstr,	NULL,	0
};

const struct name_value tracked_names[] =
{
	"cat",		"/bin/cat",	N_FREE|N_EXPORT|T_FLAG,
	"chmod",	"/bin/chmod",	N_FREE|N_EXPORT|T_FLAG,
	"cc",		"/bin/cc",	N_FREE|N_EXPORT|T_FLAG,
	"cp",		"/bin/cp",	N_FREE|N_EXPORT|T_FLAG,
	"date",		"/bin/date",	N_FREE|N_EXPORT|T_FLAG,
	"ed",		"/bin/ed",	N_FREE|N_EXPORT|T_FLAG,
#ifdef _bin_grep_
	"grep",		"/bin/grep",	N_FREE|N_EXPORT|T_FLAG,
#else
#  ifdef _usr_ucb_
	"grep",		"/usr/ucb/grep",N_FREE|N_EXPORT|T_FLAG,
#  endif /* _usr_ucb_ */
#endif	/* _bin_grep */
#ifdef _usr_bin_lp
	"lp",		"/usr/bin/lp",	N_FREE|N_EXPORT|T_FLAG,
#endif /* _usr_bin_lpr */
#ifdef _usr_bin_lpr
	"lpr",		"/usr/bin/lpr",	N_FREE|N_EXPORT|T_FLAG,
#endif /* _usr_bin_lpr */
	"ls",		"/bin/ls",	N_FREE|N_EXPORT|T_FLAG,
	"make",		"/bin/make",	N_FREE|N_EXPORT|T_FLAG,
	"mail",		"/bin/mail",	N_FREE|N_EXPORT|T_FLAG,
	"mv",		"/bin/mv",	N_FREE|N_EXPORT|T_FLAG,
	"pr",		"/bin/pr",	N_FREE|N_EXPORT|T_FLAG,
	"rm",		"/bin/rm",	N_FREE|N_EXPORT|T_FLAG,
	"sed",		"/bin/sed",	N_FREE|N_EXPORT|T_FLAG,
	"sh",		"/bin/sh",	N_FREE|N_EXPORT|T_FLAG,
#ifdef _usr_bin_vi_
	"vi",		"/usr/bin/vi",	N_FREE|N_EXPORT|T_FLAG,
#else
#  ifdef _usr_ucb_
	"vi",		"/usr/ucb/vi",	N_FREE|N_EXPORT|T_FLAG,
#  endif /* _usr_ucb_ */
#endif	/* _usr_bin_vi_ */
	"who",		"/bin/who",	N_FREE|N_EXPORT|T_FLAG,
	e_nullstr,	NULL,	0
};

/* tables */
SYSTAB tab_reserved =
{
#ifdef POSIX2
		/*  The negation operator */
		{"!",		NOTSYM},
#endif /* POSIX2 */
#ifdef NEWTEST
		{"[[",		BTSTSYM},
#endif /* NEWTEST */
		{"case",	CASYM},
		{"do",		DOSYM},
		{"done",	ODSYM},
		{"elif",	EFSYM},
		{"else",	ELSYM},
		{"esac",	ESSYM},
		{"fi",		FISYM},
		{"for",		FORSYM},
		{"function",	PROCSYM},
		{"if",		IFSYM},
		{"in",		INSYM},
		{"select",	SELSYM},
		{"then",	THSYM},
#ifndef POSIX3
		/* This will take out time keyword from POSIX shell */
		{"time",	TIMSYM},
#endif /* not POSIX3 */
		{"until",	UNSYM},
		{"while",	WHSYM},
		{"{",		BRSYM},
		{"}",		KTSYM},
		{e_nullstr,	0},
};

/*
 * The signal numbers go in the low bits and the attributes go in the high bits
 */

SYSTAB	sig_names =
{
#ifdef SIGABRT
		{"ABRT",	(SIGABRT+1)|(SIGDONE<<SIGBITS)},
#endif /*SIGABRT */
		{"ALRM",	(SIGALRM+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
		{"BUS",		(SIGBUS+1)|(SIGDONE<<SIGBITS)},
#ifdef SIGCHLD
		{"CHLD",	(SIGCHLD+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#   ifdef SIGCLD
#	if SIGCLD!=SIGCHLD
		{"CLD",		(SIGCLD+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#	endif
#   endif	/* SIGCLD */
#else
#   ifdef SIGCLD
		{"CLD",		(SIGCLD+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#   endif	/* SIGCLD */
#endif	/* SIGCHLD */
#ifdef SIGCONT
		{"CONT",	(SIGCONT+1)},
#endif	/* SIGCONT */
		{"DEBUG",	(DEBUGTRAP+1)},
#ifdef SIGEMT
		{"EMT",		(SIGEMT+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGEMT */
		{"ERR",		(ERRTRAP+1)},
		{"EXIT",	1},
		{"FPE",		(SIGFPE+1)|(SIGDONE<<SIGBITS)},
		{"HUP",		(SIGHUP+1)|(SIGDONE<<SIGBITS)},
		{"ILL",		(SIGILL+1)|(SIGDONE<<SIGBITS)},
		{"INT",		(SIGINT+1)|(SIGCAUGHT<<SIGBITS)},
#ifdef SIGIO
		{"IO",		(SIGIO+1)},
#endif	/* SIGIO */
#ifndef POSIX2
/* we want to have SIGABRT override SIGIOT for POSIX */
		{"IOT",		(SIGIOT+1)|(SIGDONE<<SIGBITS)},
#endif /* POSIX2 */
		{"KILL",	(SIGKILL+1)},
#ifdef SIGLAB
		{"LAB",		(SIGLAB+1)},
#endif	/* SIGLAB */
#ifdef SIGLOST
		{"LOST",	(SIGLOST+1)},
#endif	/* SIGLOST */
#ifdef POSIX2
		/*  for SIGNULL in kill */
		{"NULL",	1},
#endif /* POSIX2 */
#ifdef SIGPHONE
		{"PHONE",	(SIGPHONE+1)},
#endif	/* SIGPHONE */
		{"PIPE",	(SIGPIPE+1)|(SIGDONE<<SIGBITS)},
#ifdef SIGPOLL
		{"POLL",	(SIGPOLL+1)},
#endif	/* SIGPOLL */
#ifdef SIGPROF
		{"PROF",	(SIGPROF+1)},
#endif	/* SIGPROF */
#ifdef SIGPWR
#   if SIGPWR>0
		{"PWR",		(SIGPWR+1)},
#   endif
#endif	/* SIGPWR */
		{"QUIT",	(SIGQUIT+1)|((SIGCAUGHT|SIGIGNORE)<<SIGBITS)},
		{"SEGV",	(SIGSEGV+1)},
#ifdef SIGSTOP
		{"STOP",	(SIGSTOP+1)},
#endif	/* SIGSTOP */
		{"SYS",		(SIGSYS+1)|(SIGDONE<<SIGBITS)},
		{"TERM",	(SIGTERM+1)|(SIGDONE<<SIGBITS)},
#ifdef SIGTINT
		{"TINT",	(SIGTINT+1)},
#endif	/* SIGTINT */
		{"TRAP",	(SIGTRAP+1)|(SIGDONE<<SIGBITS)},
#ifdef SIGTSTP
		{"TSTP",	(SIGTSTP+1)},
#endif	/* SIGTSTP */
#ifdef SIGTTIN
		{"TTIN",	(SIGTTIN+1)},
#endif	/* SIGTTIN */
#ifdef SIGTTOU
		{"TTOU",	(SIGTTOU+1)},
#endif	/* SIGTTOU */
#ifdef SIGURG
		{"URG",		(SIGURG+1)},
#endif	/* SIGURG */
#ifdef SIGUSR1
		{"USR1",	(SIGUSR1+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGUSR1 */
#ifdef SIGUSR2
		{"USR2",	(SIGUSR2+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGUSR2 */
#ifdef SIGVTALRM
		{"VTALRM",	(SIGVTALRM+1)},
#endif	/* SIGVTALRM */
#ifdef SIGWINCH
		{"WINCH",	(SIGWINCH+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#endif	/* SIGWINCH */
#ifdef SIGWIND
		{"WIND",	(SIGWIND+1)},
#endif	/* SIGWIND */
#ifdef SIGXCPU
		{"XCPU",	(SIGXCPU+1)},
#endif	/* SIGXCPU */
#ifdef SIGXFSZ
		{"XFSZ",	(SIGXFSZ+1)|((SIGCAUGHT|SIGIGNORE)<<SIGBITS)},
#endif	/* SIGXFSZ */
		{e_nullstr,	0}
};

SYSTAB	sig_messages =
{
#ifdef SIGABRT
		{"Abort",			(SIGABRT+1)},
#endif /*SIGABRT */
		{"Alarm call",			(SIGALRM+1)},
		{"Bus error",			(SIGBUS+1)},
#ifdef SIGCHLD
		{"Child stopped or terminated",	(SIGCHLD+1)},
#   ifdef SIGCLD
#	if SIGCLD!=SIGCHLD
		{"Death of Child", 		(SIGCLD+1)},
#	endif
#   endif	/* SIGCLD */
#else
#   ifdef SIGCLD
		{"Death of Child", 		(SIGCLD+1)},
#   endif	/* SIGCLD */
#endif	/* SIGCHLD */
#ifdef SIGCONT
		{"Stopped process continued",	(SIGCONT+1)},
#endif	/* SIGCONT */
#ifdef SIGEMT
		{"EMT trap",			(SIGEMT+1)},
#endif	/* SIGEMT */
		{"Floating exception",		(SIGFPE+1)},
		{"Hangup",			(SIGHUP+1)},
		{"Illegal instruction",		(SIGILL+1)},
#ifdef JOBS
		{"Interrupt",			(SIGINT+1)},
#else
		{e_nullstr,			(SIGINT+1)},
#endif	/* JOBS */
#ifdef SIGIO
		{"IO signal",			(SIGIO+1)},
#endif	/* SIGIO */
		{"Abort",			(SIGIOT+1)},
		{"Killed",			(SIGKILL+1)},
		{"Quit",			(SIGQUIT+1)},
#ifdef JOBS
		{"Broken Pipe",			(SIGPIPE+1)},
#else
		{e_nullstr,			(SIGPIPE+1)},
#endif	/* JOBS */
#ifdef SIGPROF
		{"Profiling time alarm",	(SIGPROF+1)},
#endif	/* SIGPROF */
#ifdef SIGPWR
#   if SIGPWR>0
		{"Power fail",			(SIGPWR+1)},
#   endif
#endif	/* SIGPWR */
		{"Memory fault",		(SIGSEGV+1)},
#ifdef SIGSTOP
#ifdef POSIX3
		{"Stopped (SIGSTOP)",		(SIGSTOP+1)},
#else
		{"Stopped (signal)",		(SIGSTOP+1)},
#endif /* POSIX3 */
#endif	/* SIGSTOP */
		{"Bad system call", 		(SIGSYS+1)},
		{"Terminated",			(SIGTERM+1)},
#ifdef SIGTINT
#   ifdef JOBS
		{"Interrupt",			(SIGTINT+1)},
#   else
		{e_nullstr,			(SIGTINT+1)},
#   endif /* JOBS */
#endif	/* SIGTINT */
		{"Trace/BPT trap",		(SIGTRAP+1)},
#ifdef SIGTSTP
		{"Stopped",			(SIGTSTP+1)},
#endif	/* SIGTSTP */
#ifdef SIGTTIN
#ifdef POSIX3
		{"Stopped (SIGTTIN)",		(SIGTTIN+1)},
#else
		{"Stopped (tty input)",		(SIGTTIN+1)},
#endif /* POSIX3 */
#endif	/* SIGTTIN */
#ifdef SIGTTOU
#ifdef POSIX3
		{"Stopped (SIGTTOU)",		(SIGTTOU+1)},
#else
		{"Stopped (tty output)",	(SIGTTOU+1)},
#endif /* POSIX3 */
#endif	/* SIGTTOU */
#ifdef SIGURG
		{"Socket interrupt",		(SIGURG+1)},
#endif	/* SIGURG */
#ifdef SIGUSR1
		{"User signal 1",		(SIGUSR1+1)},
#endif	/* SIGUSR1 */
#ifdef SIGUSR2
		{"User signal 2",		(SIGUSR2+1)},
#endif	/* SIGUSR2 */
#ifdef SIGVTALRM
		{"Virtual time alarm",		(SIGVTALRM+1)},
#endif	/* SIGVTALRM */
#ifdef SIGWINCH
		{"Window size change", 		(SIGWINCH+1)},
#endif	/* SIGWINCH */
#ifdef SIGXCPU
		{"Exceeded CPU time limit",	(SIGXCPU+1)},
#endif	/* SIGXCPU */
#ifdef SIGXFSZ
		{"Exceeded file size limit",	(SIGXFSZ+1)},
#endif	/* SIGXFSZ */
#ifdef SIGLOST
		{"Resources lost", 		(SIGLOST+1)},
#endif	/* SIGLOST */
#ifdef SIGLAB
		{"Security label changed",	(SIGLAB+1)},
#endif	/* SIGLAB */
		{e_nullstr,	0}
};

SYSTAB tab_options=
{
	{"allexport",		Allexp},
	{"bgnice",		Bgnice},
#ifdef HPBRACE
        {"braces",		Hpbraces},
#endif /* HPBRACE */
	{"emacs",		Emacs},
	{"errexit",		Errflg},
	{"gmacs",		Gmacs},
	{"ignoreeof",		Noeof},
#ifdef apollo
	{"inprocess",		Inproc},
#endif	/* apollo */
	{"interactive",		Intflg},
	{"keyword",		Keyflg},
	{"markdirs",		Markdir},
	{"monitor",		Monitor},
	{"noexec",		Noexec},
	{"noclobber",		Noclob},
	{"noglob",		Noglob},
	{"nolog",		Nolog},
	{"nounset",		Noset},
	{"privileged",		Privmod},
	{"restricted",		Rshflg},
	{"trackall",		Hashall},
	{"verbose",		Readpr},
	{"vi",			Editvi},
	{"viraw",		Viraw},
	{"xtrace",		Execpr},
	{e_nullstr,		0}
};

#ifdef _sys_resource_
#   ifndef included_sys_time_
#	include <sys/time.h>
#   endif
#   include	<sys/resource.h>/* needed for ulimit */
#   define	LIM_FSIZE	RLIMIT_FSIZE
#   define	LIM_DATA	RLIMIT_DATA
#   define	LIM_STACK	RLIMIT_STACK
#   define	LIM_CORE	RLIMIT_CORE
#   define	LIM_CPU		RLIMIT_CPU
#   ifdef RLIMIT_RSS
#	define	LIM_MAXRSS	RLIMIT_RSS
#   endif /* RLIMIT_RSS */
#else
#   ifdef VLIMIT
#	include	<sys/vlimit.h>
#   endif /* VLIMIT */
#endif	/* _sys_resource_ */

#ifdef LIM_CPU
#   define size_resource(a,b) ((a)|((b)<<11))	
SYSTAB limit_names =
{
	{"time(seconds)       ",	size_resource(1,LIM_CPU)},
	{"file(blocks)        ",	size_resource(512,LIM_FSIZE)},
	{"data(kbytes)        ",	size_resource(1024,LIM_DATA)},
	{"stack(kbytes)       ",	size_resource(1024,LIM_STACK)},
#   ifdef LIM_MAXRSS
	{"memory(kbytes)      ",	size_resource(1024,LIM_MAXRSS)},
#   else
	{"memory(kbytes)      ",	size_resource(1024,0)},
#   endif /* LIM_MAXRSS */
	{"coredump(blocks)    ",	size_resource(512,LIM_CORE)},
#   ifdef RLIMIT_NOFILE
	{"nofiles(descriptors)",	size_resource(1,RLIMIT_NOFILE)},
#   else
	{"nofiles(descriptors)",	size_resource(1,0)},
#   endif /* RLIMIT_NOFILE */
#   ifdef RLIMIT_VMEM
	{"vmemory(kbytes)     ",	size_resource(1024,RLIMIT_VMEM)}
#   else
	{"vmemory(kbytes)     ",	size_resource(1024,0)}
#   endif /* RLIMIT_VMEM */
};
#endif	/* LIM_CPU */

SYSTAB	tab_builtins=
{
		{".",		SYSDOT},
		{":",		SYSNULL},
		{"[",		SYSTEST},
		{ "alias",	SYSALIAS|SYSDECLARE},
#ifdef SIGTSTP
		{ "bg",		SYSBG},
#endif /* SIGTSTP */
		{"break",	SYSBREAK},
		{"cd",		SYSCD},
#ifdef POSIX2
		{"command",	SYSCOMMAND},
#endif /* POSIX2 */
		{"continue",	SYSCONT},
		{"echo",	SYSECHO},
		{"exec",	SYSEXEC},
		{"exit",	SYSEXIT},
		{"export",	SYSXPORT|SYSDECLARE},
		{"eval",	SYSEVAL},
		{"fc",		SYSFC},
#ifdef SIGTSTP
		{"fg",		SYSFG},
#endif /* SIGTSTP */
		{"getopts",	SYSGETOPTS},
#ifdef apollo
		{"inlib",	SYSINLIB},
		{"inprocess",	SYSINPROCESS},
#endif	/* apollo */
#ifdef JOBS
		{"jobs",	SYSJOBS},
		{"kill",	SYSKILL},
#endif	/* JOBS */
		{"let",		SYSLET},
		{"login",	SYSLOGIN},
#ifdef RFA
		{"netunam",	SYSNETUNAM},
#endif /* RFA */
#ifdef _bin_newgrp_
		{"newgrp",	SYSLOGIN},
#endif	/* _bin_newgrp_ */
		{"print",	SYSPRINT},
		{"pwd",		SYSPWD},
		{"read",	SYSREAD},
		{"readonly",	SYSRDONLY|SYSDECLARE},
		{"return",	SYSRETURN},
		{"set",		SYSSET},
		{"shift",	SYSSHFT},
#ifdef OLDTEST
		{"test",	SYSTEST},
#endif /* OLDTEST */
		{"times",	SYSTIMES},
		{"trap",	SYSTRAP},
		{"typeset",	SYSTYPESET|SYSDECLARE},
		{"ulimit",	SYSULIMIT},
		{"umask",	SYSUMASK},
		{"unalias",	SYSUNALIAS},
		{"unset",	SYSUNSET},
#ifdef FS_3D
		{"vmap",	SYSVMAP},
		{"vpath",	SYSVPATH},
#endif /* FS_3D */
		{"wait",	SYSWAIT},
		{"whence",	SYSWHENCE},
		{e_nullstr,		0}
};

#ifdef HP_BUILTIN
/*
 *  These built-ins are turned off/on with the 
 *  set +/- hp_builtin
 */

SYSTAB tab_hp_builtins =
{
		{"getlocale",	HP_GETLOCALE},
		{"pgrp",	HP_PGRP},
		{"pid",		HP_PID},
		{"ttygrp",	HP_TTYGRP},
		{"version",	HP_VERSION},
		{e_nullstr,	0}
};
#endif /* HP_BUILTIN */

SYSTAB	test_optable =
{
		{"!=",		TEST_SNE},
		{"-a",		TEST_AND},
		{"-ef",		TEST_EF},
		{"-eq",		TEST_EQ},
		{"-ge",		TEST_GE},
		{"-gt",		TEST_GT},
		{"-le",		TEST_LE},
		{"-lt",		TEST_LT},
		{"-ne",		TEST_NE},
		{"-nt",		TEST_NT},
		{"-o",		TEST_OR},
		{"-ot",		TEST_OT},
		{"=",		TEST_SEQ},
#ifdef NEWTEST
		{"<",		TEST_SLT},
		{">",		TEST_SGT},
		{"]]",		TEST_END},
#endif /* NEWTEST */
		{e_nullstr,	0}
};

SYSTAB	tab_attributes =
{
		{"export",	N_EXPORT},
		{"readonly",	N_RDONLY},
		{"tagged",	T_FLAG},
#ifdef FLOAT
		{"exponential",	(N_DOUBLE|N_INTGER|N_EXPNOTE)},
		{"float",	(N_DOUBLE|N_INTGER)},
#endif /* FLOAT */
		{"long",	(L_FLAG|N_INTGER)},
		{"unsigned",	(N_UNSIGN|N_INTGER)},
		{"function",	(N_BLTNOD|N_INTGER)},
		{"integer",	N_INTGER},
		{"filename",	N_HOST},
		{"lowercase",	N_UTOL},
		{"zerofill",	N_ZFILL},
		{"leftjust",	N_LJUST},
		{"rightjust",	N_RJUST},
		{"uppercase",	N_LTOU},
		{e_nullstr,	0}
};

SYSTAB ctype_funcs =
{
		{"alnum",	CTYPE_ALNUM},
		{"alpha",	CTYPE_ALPHA},
		{"ascii",	CTYPE_ASCII},
		{"cntrl",	CTYPE_CNTRL},
		{"digit",	CTYPE_DIGIT},
		{"graph",	CTYPE_GRAPH},
		{"lower",	CTYPE_LOWER},
		{"print",	CTYPE_PRINT},
		{"punct",	CTYPE_PUNCT},
		{"space",	CTYPE_SPACE},
		{"upper",	CTYPE_UPPER},
		{"xdigit",	CTYPE_XDIGIT},
		{e_nullstr,	0}
};

#ifdef _sgtty_
#   ifdef _SELECT_
	const int tty_speeds[] = {0, 50, 75, 110, 134, 150, 200, 300,
			600,1200,1800,2400,9600,19200,0};
#   endif /* _SELECT_ */
#endif /* _sgtty_ */

#ifdef HP_DEBUG
#include "debug.h"
/*
 *  This stuff is for the debug options that can be turned
 *  on/off with set -D or set +D option.  See also sh/args.c.
 *
 *  At the place you want to put debugging stuff, make something
 *  like:
 *       #ifdef HP_DEBUG
 *	     if(hp_debug & DEBUG_OPTION)
 *	     {
 *		  do debugging stuff
 *	     }
 *	 #endif
 *
 *  Make sure DEBUG_OPTION is defined here and is in the
 *  debug_options SYSTAB.  Then at runtime, when you want 
 *  turn on DEBUG_OPTION, do a "set -D debug_option" and
 *  it will be turned on.  To turn it off do a "set +D debug_option".
 */


/*
 *  This is the system table that associates an option
 *  to set -D with the appropriate bit in hp_debug.
 */
SYSTAB debug_options = {
	{"all",		ALL_DEBUG},
	{"fork",	FORK_DEBUG},
	{"z1",		Z1_DEBUG},
	{"z2",		Z2_DEBUG},
	{"z3",		Z3_DEBUG},
	{e_nullstr,	0}
};

#endif /* HP_DEBUG */
