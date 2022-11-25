/* @(#) $Revision: 70.1 $ */
/**************************************************************
 * C shell
 ***************************************************************/

#include "sh.local.h"
#include <signal.h>

#define NL_SETN 13		/* same set as sh.proc.c */

extern	int doalias();
#ifdef SIGTSTP
extern	int dobg();
#endif
extern	int dobreak();
extern	int dochngd();
extern	int docontin();
extern	int dodirs();
extern	int doecho();
extern	int doelse();
extern	int doend();
extern	int doendif();
extern	int doendsw();
extern	int doeval();
extern	int doexit();
#ifdef SIGTSTP
extern	int dofg();
#endif
extern	int doforeach();
extern	int doglob();
extern	int dogoto();
extern	int dohash();
extern	int dohist();
extern	int doif();
extern	int dojobs();
extern	int dokill();
extern	int dolet();
#ifdef VLIMIT
extern	int dolimit();
#endif
extern	int dologin();
extern	int dologout();
#ifdef RFA
extern  int donetunam();
#endif /* RFA */
extern	int donewgrp();
extern	int donice();
extern	int donotify();
extern	int donohup();
extern	int doonintr();
extern	int dopopd();
extern	int dopushd();
extern	int dorepeat();
extern	int doset();
extern	int dosetenv();
extern	int dosource();
extern	int dostop();
#ifdef SIGTSTP
extern	int dosuspend();
#endif
extern	int doswbrk();
extern	int doswitch();
extern	int dotime();
#ifdef VLIMIT
extern	int dounlimit();
#endif
extern	int doumask();
extern	int dowait();
extern	int dowhile();
extern	int dozip();
extern	int execash();
extern	int goodbye();
extern	int hashstat();
extern	int shift();
#ifdef debug
extern	int showall();
#endif
extern	int unalias();
extern	int dounhash();
extern	int unset();
extern	int dounsetenv();

/*  INF changed to NCARGS/3; (was 1000); DSDe412639 */

#define	INF	NCARGS/3

unsigned short nullstr[] = {0};

struct	biltins {
	char	*bname;
	int	(*bfunct)();
	short	minargs, maxargs;
} bfunc[] = {
	"@",		dolet,		0,	INF,
	"alias",	doalias,	0,	INF,
#ifdef debug
	"alloc",	showall,	0,	1,
#endif
#ifdef SIGTSTP
	"bg",		dobg,		0,	INF,
#endif
	"break",	dobreak,	0,	0,
	"breaksw",	doswbrk,	0,	0,
	"bye",		goodbye,	0,	0,
	"case",		dozip,		0,	1,
	"cd",		dochngd,	0,	1,
	"chdir",	dochngd,	0,	1,
	"continue",	docontin,	0,	0,
	"default",	dozip,		0,	0,
	"dirs",		dodirs,		0,	1,
	"echo",		doecho,		0,	INF,
	"else",		doelse,		0,	INF,
	"end",		doend,		0,	0,
	"endif",	dozip,		0,	0,
	"endsw",	dozip,		0,	0,
	"eval",		doeval,		0,	INF,
	"exec",		execash,	1,	INF,
	"exit",		doexit,		0,	INF,
#ifdef SIGTSTP
	"fg",		dofg,		0,	INF,
#endif
	"foreach",	doforeach,	3,	INF,
	"gd",		dopushd,	0,	1,
	"glob",		doglob,		0,	INF,
	"goto",		dogoto,		1,	1,
	"hashstat",	hashstat,	0,	0,
	"history",	dohist,		0,	3,
	"if",		doif,		1,	INF,
	"jobs",		dojobs,		0,	1,
	"kill",		dokill,		1,	INF,
#ifdef VLIMIT
	"limit",	dolimit,	0,	3,
#endif
	"login",	dologin,	0,	1,
	"logout",	dologout,	0,	0,
#ifdef RFA
	"netunam",      donetunam,      2,      2,
#endif
	"newgrp",	donewgrp,	0,	2,
	"nice",		donice,		0,	INF,
	"nohup",	donohup,	0,	INF,
	"notify",	donotify,	0,	INF,
	"onintr",	doonintr,	0,	2,
	"popd",		dopopd,		0,	1,
	"pushd",	dopushd,	0,	1,
	/*"rd",		dopopd,		0,	1, */  /* This built-in is not documented */
	"rehash",	dohash,		0,	0,
	"repeat",	dorepeat,	2,	INF,
	"set",		doset,		0,	INF,
	"setenv",	dosetenv,	2,	2,
	"shift",	shift,		0,	1,
	"source",	dosource,	1,	2,
	"stop",		dostop,		1,	INF,
#ifdef SIGTSTP
	"suspend",	dosuspend,	0,	0,
#endif
	"switch",	doswitch,	1,	INF,
	"time",		dotime,		0,	INF,
	"umask",	doumask,	0,	1,
	"unalias",	unalias,	1,	INF,
	"unhash",	dounhash,	0,	0,
#ifdef VLIMIT
	"unlimit",	dounlimit,	0,	INF,
#endif
	"unset",	unset,		1,	INF,
	"unsetenv",	dounsetenv,	1,	INF,
	"wait",		dowait,		0,	0,
	"while",	dowhile,	1,	INF,
	0,		0,		0,	0,
};

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

struct srch {
	char	*s_name;
	short	s_value;
} srchn[] = {
	"@",		ZLET,
	"break",	ZBREAK,
	"breaksw",	ZBRKSW,
	"case",		ZCASE,
	"default", 	ZDEFAULT,
	"else",		ZELSE,
	"end",		ZEND,
	"endif",	ZENDIF,
	"endsw",	ZENDSW,
	"exit",		ZEXIT,
	"foreach", 	ZFOREACH,
	"goto",		ZGOTO,
	"if",		ZIF,
	"label",	ZLABEL,
	"set",		ZSET,
	"switch",	ZSWITCH,
	"while",	ZWHILE,
	0,		0,
};

struct	mesg {
	char	*iname;
	char	*pname;
} mesg[] = {
	0,       0,
	"HUP",
		"Hangup",			/* catgets 1 */
	"INT",
		"Interrupt",			/* catgets 2 */
	"QUIT",
		"Quit",				/* catgets 3 */
	"ILL",
		"Illegal instruction",		/* catgets 4 */
	"TRAP",
		"Trace/BPT trap",		/* catgets 5 */
	"IOT",
		"IOT trap",			/* catgets 6 */
	"EMT",
		"EMT trap",			/* catgets 7 */
	"FPE",
		"Floating exception",		/* catgets 8 */
	"KILL",
		"Killed",			/* catgets 9 */
	"BUS",
		"Bus error",			/* catgets 10 */
	"SEGV",
		"Segmentation fault",		/* catgets 11 */
	"SYS",
		"Bad system call",		/* catgets 12 */
	"PIPE",
		"Broken pipe",			/* catgets 13 */
	"ALRM",
		"Alarm clock",			/* catgets 14 */
	"TERM",
		"Terminated",			/* catgets 15 */
	"USR1",
		"Signal 16",			/* catgets 16 */
	"USR2",
		"Signal 17",			/* catgets 17 */
	"CLD",
		"Child exited",			/* catgets 18 */
	"PWR",
		"Power-fail restart",		/* catgets 19 */
	"VTALRM",
		"Virtual Timer Alarm",		/* catgets 20 */
	"PROF",
		"Profiling Timer Alarm",	/* catgets 21 */
	"IO",
		"Async I/O Signal",		/* catgets 22 */
#ifdef SIGWINCH
	"WINCH",
		"Window Signal",		/* catgets 23 */
#else
	"WINDOW",
		"Window Signal",		/* catgets 23 */
#endif /* SIGWINCH */
#ifdef SIGTSTP
	"STOP",
		"Stopped",			/* catgets 24 */
	"TSTP",
		"Stopped",			/* catgets 25 */
	"CONT",
		"Continued",			/* catgets 26 */
	"TTIN",
		"Stopped (tty input)",		/* catgets 27 */
	"TTOU",
		"Stopped (tty output)",		/* catgets 28 */
#else
	0,      "Signal 24",
	0,      "Signal 25",
	0,      "Signal 26",
	0,      "Signal 27",
	0,	"Signal 28",
#endif
	0,	"Signal 29",			/* catgets 29 */
	0,	"Signal 30",			/* catgets 30 */
	0,	"Signal 31",			/* catgets 31 */
	0,	"Signal 32"			/* catgets 32 */
};

#ifndef NONLS
/* The following are some CHAR string constants which we initialize here*/
#define CHAR unsigned short int
CHAR	CH_home[] = {'h','o','m','e',0};
CHAR	CH_autologout[] = {'a','u','t','o','l','o','g','o','u','t',0};
CHAR	CH_listpathnum[] = {'l','i','s','t','p','a','t','h','n','u','m',0};
CHAR	CH_path[] = {'p','a','t','h',0};
CHAR	CH_status[] = {'s','t','a','t','u','s',0};
CHAR	CH_term[] = {'t','e','r','m',0};
CHAR	CH_user[] = {'u','s','e','r',0};
CHAR    CH_HOME[] = {'H','O','M','E',0};
CHAR    CH_PATH[] = {'P','A','T','H',0};
CHAR    CH_TERM[] = {'T','E','R','M',0};
CHAR    CH_USER[] = {'U','S','E','R',0};
CHAR    CH_argv[] = {'a','r','g','v',0};
CHAR    CH_cdpath[] = {'c','d','p','a','t','h',0};
CHAR    CH_child[] = {'c','h','i','l','d',0};
CHAR    CH_cwd[] = {'c','w','d',0};
CHAR    CH_echo[] = {'e','c','h','o',0};
CHAR    CH_histchars[] = {'h','i','s','t','c','h','a','r','s',0};
CHAR    CH_history[] = {'h','i','s','t','o','r','y',0};
CHAR    CH_ignoreeof[] = {'i','g','n','o','r','e','e','o','f',0};
CHAR    CH_mail[] = {'m','a','i','l',0};
CHAR    CH_noclobber[] = {'n','o','c','l','o','b','b','e','r',0};
CHAR    CH_noglob[] = {'n','o','g','l','o','b',0};
CHAR    CH_nonomatch[] = {'n','o','n','o','m','a','t','c','h',0};
CHAR    CH_prompt[] = {'p','r','o','m','p','t',0};
CHAR    CH_savehist[] = {'s','a','v','e','h','i','s','t',0};
CHAR    CH_shell[] = {'s','h','e','l','l',0};
CHAR    CH_verbose[] = {'v','e','r','b','o','s','e',0};
CHAR	CH_time[] = {'t','i','m','e',0};
CHAR	CH_notify[] = {'n','o','t','i','f','y',0};
CHAR	CH_zero[] = {'0',0};
CHAR	CH_one[] = {'1',0};
CHAR	CH_dot[] = {'.',0};
CHAR 	CH_slash[] = {'/',0};
#ifdef SIGWINCH
CHAR	CH_columns[] = {'C','O','L','U','M','N','S',0};
CHAR    CH_lines[] = { 'L','I','N','E','S',0};
#endif /* SIGWINCH */
#if defined(DISKLESS) || defined(DUX)
CHAR	CH_hidden[] = {'h','i','d','d','e','n',0};
#endif /* DISKLESS || DUX */
#endif
