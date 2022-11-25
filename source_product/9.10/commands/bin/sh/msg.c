/* @(#) $Revision: 66.2 $ */
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"sym.h"

#ifdef NLS
#define NL_SETN 1
#endif

/*
 * error messages
 */
char	badopt[]	= "bad option(s)";		/* nl_msg 601 */
char	mailmsg[]	= "you have mail\n";		/* nl_msg 602 */
char	nospace[]	= "no space";			/* nl_msg 603 */
char	nostack[]	= "no stack space";		/* nl_msg 604 */
char	synmsg[]	= "syntax error";		/* nl_msg 605 */

char	badsig[]	= "unknown signal";		/* nl_msg 606 */
char	badnum[]	= "bad number";			/* nl_msg 607 */
char	badparam[]	= "parameter null or not set";	/* nl_msg 608 */
char	unset[]		= "parameter not set";		/* nl_msg 609 */
char	badsub[]	= "bad substitution";		/* nl_msg 610 */
char	badcreate[]	= "cannot create";		/* nl_msg 611 */
char	nofork[]	= "fork failed - too many processes";	/* nl_msg 612 */
char	noswap[]	= "cannot fork: no swap space";	/* nl_msg 613 */
char	restricted[]	= "restricted";			/* nl_msg 614 */
char	piperr[]	= "cannot make pipe";		/* nl_msg 615 */
char	badopen[]	= "cannot open";		/* nl_msg 616 */
char	coredump[]	= " - core dumped";		/* nl_msg 617 */
char	arglist[]	= "arg list too long";		/* nl_msg 618 */
char	txtbsy[]	= "text busy";			/* nl_msg 619 */
char	toobig[]	= "too big";			/* nl_msg 620 */
char	badexec[]	= "cannot execute";		/* nl_msg 621 */
char	notfound[]	= "not found";			/* nl_msg 622 */
char	badfile[]	= "bad file number";		/* nl_msg 623 */
char	badshift[]	= "cannot shift";		/* nl_msg 624 */
char	baddir[]	= "bad directory";		/* nl_msg 625 */
char	badtrap[]	= "bad trap";			/* nl_msg 626 */
char	wtfailed[]	= "is read only";		/* nl_msg 627 */
char	notid[]		= "is not an identifier";	/* nl_msg 628 */
char 	badulimit[]	= "Bad ulimit";			/* nl_msg 629 */
char	badreturn[] 	= "cannot return when not in function";	/* nl_msg 630 */
char	badexport[] 	= "cannot export functions";	/* nl_msg 631 */
char	badunset[] 	= "cannot unset";		/* nl_msg 632 */
char	nohome[]	= "no home directory";		/* nl_msg 633 */
char 	badperm[]	= "execute permission denied";	/* nl_msg 634 */
char	longpwd[]	= "sh error: pwd too long";	/* nl_msg 635 */
char	ses_failed[]	= "sigemptyset() failed";	/* nl_msg 661 */
char	spm_failed[]	= "sigprocmask() failed";	/* nl_msg 662 */
char	sds_failed[]	= "sigdelset() failed";		/* nl_msg 663 */

/*
 * messages for 'builtin' functions
 */
char	btest[]		= "test";
char	badop[]		= "unknown operator ";		/* nl_msg 636 */

/*
 * built in names
 */
#ifndef NLS
char	pathname[]	= "PATH";
char	cdpname[]	= "CDPATH";
char	homename[]	= "HOME";
char	mailname[]	= "MAIL";
char	ifsname[]	= "IFS";
char	ps1name[]	= "PS1";
char	ps2name[]	= "PS2";
char	mchkname[]	= "MAILCHECK";
char	acctname[]  	= "SHACCT";
char	mailpname[]	= "MAILPATH";
#else /* NLS */
tchar	pathname[]	= {'P','A','T','H',0};
tchar	cdpname[]	= {'C','D','P','A','T','H',0};
tchar	homename[]	= {'H','O','M','E',0};
tchar	mailname[]	= {'M','A','I','L',0};
tchar	ifsname[]	= {'I','F','S',0};
tchar	ps1name[]	= {'P','S','1',0};
tchar	ps2name[]	= {'P','S','2',0};
tchar	mchkname[]	= {'M','A','I','L','C','H','E','C','K',0};
tchar	acctname[]  	= {'S','H','A','C','C','T',0};
tchar	mailpname[]	= {'M','A','I','L','P','A','T','H',0};
#endif /* not NLS */

/*
 * string constants
 */
#ifndef NLS
char	nullstr[]	= "";
char	sptbnl[5]	= " \t\n";
char	defpath[]	= ":/bin:/usr/bin";
char	colon[]		= ": ";
char	minus[]		= "-";
char	endoffile[]	= "end of file";		/* nl_msg 637 */
char	unexpected[] 	= " unexpected";		/* nl_msg 638 */
char	atline[]	= " at line ";			/* nl_msg 639 */
char	devnull[]	= "/dev/null";
char	execpmsg[]	= "+ ";
char	readmsg[]	= "> ";
char	stdprompt[]	= "$ ";
char	supprompt[]	= "# ";
char	profile[]	= ".profile";
char	sysprofile[]	= "/etc/profile";
char	shell[6]	= "SHELL";
#else /* NLS */
tchar	nullstr[]	= {0};
tchar	sptbnl[5]	= {' ','\t','\n',0, 0};
tchar	defpath[]	= {':','/','b','i','n',':','/','u','s','r','/','b','i','n',0};
char	colon[]		= ": ";
tchar	minus[]		= {'-',0};
char	endoffile[]	= "end of file";		/* nl_msg 637 */
char	unexpected[] 	= " unexpected";		/* nl_msg 638 */
char	atline[]	= " at line ";			/* nl_msg 639 */
tchar	devnull[]	= {'/','d','e','v','/','n','u','l','l',0};
char	execpmsg[]	= "+ ";
tchar	readmsg[]	= {'>',' ',0};
tchar	stdprompt[]	= {'$',' ',0};
tchar	supprompt[]	= {'#',' ',0};
tchar	profile[]	= {'.','p','r','o','f','i','l','e',0};
tchar	sysprofile[]	= {'/','e','t','c','/','p','r','o','f','i','l','e',0};
tchar	shell[6] 	= {'S','H','E','L','L',0};
tchar	langvar[5] 	= {'L','A','N','G',0};
tchar	lccollate[11] 	= {'L','C','_','C','O','L','L','A','T','E',0};
tchar	lcctype[9] 	= {'L','C','_','C','T','Y','P','E',0};
tchar	lcmonetary[12] 	= {'L','C','_','M','O','N','E','T','A','R','Y',0};
tchar	lcnumeric[11] 	= {'L','C','_','N','U','M','E','R','I','C',0};
tchar	lctime[8] 	= {'L','C','_','T','I','M','E',0};
#endif /* not NLS */

/*
 * tables
 */

#ifndef NLS
struct sysnod reserved[] =
{
	{ "case",	CASYM	},
	{ "do",		DOSYM	},
	{ "done",	ODSYM	},
	{ "elif",	EFSYM	},
	{ "else",	ELSYM	},
	{ "esac",	ESSYM	},
	{ "fi",		FISYM	},
	{ "for",	FORSYM	},
	{ "if",		IFSYM	},
	{ "in",		INSYM	},
	{ "then",	THSYM	},
	{ "until",	UNSYM	},
	{ "while",	WHSYM	},
	{ "{",		BRSYM	},
	{ "}",		KTSYM	}
};
#else /* NLS */
struct sysnod reserved[] =
{
	 (tchar *)"\0c\0a\0s\0e\0",	CASYM,
	 (tchar *)"\0d\0o\0",		DOSYM,
	 (tchar *)"\0d\0o\0n\0e\0",	ODSYM,
	 (tchar *)"\0e\0l\0i\0f\0",	EFSYM,
	 (tchar *)"\0e\0l\0s\0e\0",	ELSYM,
	 (tchar *)"\0e\0s\0a\0c\0",	ESSYM,
	 (tchar *)"\0f\0i\0",		FISYM,
	 (tchar *)"\0f\0o\0r\0",	FORSYM,
	 (tchar *)"\0i\0f\0",		IFSYM,
	 (tchar *)"\0i\0n\0",		INSYM,
	 (tchar *)"\0t\0h\0e\0n\0",	THSYM,
	 (tchar *)"\0u\0n\0t\0i\0l\0",	UNSYM,
	 (tchar *)"\0w\0h\0i\0l\0e\0",	WHSYM,
	 (tchar *)"\0{\0",		BRSYM,
	 (tchar *)"\0}\0",		KTSYM
};
#endif /* NLS */

int no_reserved = 15;

char	*sysmsg[] =
{
	0,
	"Hangup",			/* nl_msg 640 */
	0,				/* Interrupt */
	"Quit",				/* nl_msg 641 */
	"Illegal instruction",		/* nl_msg 642 */
	"Trace/BPT trap",		/* nl_msg 643 */
	"abort",			/* nl_msg 644 */
	"EMT trap",			/* nl_msg 646 */
	"Floating exception",		/* nl_msg 647 */
	"Killed",			/* nl_msg 648 */
	"Bus error",			/* nl_msg 649 */
	"Memory fault",			/* nl_msg 650 */
	"Bad system call",		/* nl_msg 651 */
	0,				/* Broken pipe */
	"Alarm call",			/* nl_msg 652 */
	"Terminated",			/* nl_msg 653 */
	"Signal 16",			/* nl_msg 654 */
	"Signal 17",			/* nl_msg 655 */
	"Child death",			/* nl_msg 656 */
	"Power Fail"			/* nl_msg 657 */
};

int	nsysmsg = sizeof(sysmsg)/sizeof(char *);

char	export[] = "export";		/* nl_msg 658 */
char	duperr[] = "cannot dup"; 	/* duperr is never used  - mn */
char	readonly[] = "readonly";	/* nl_msg 659 */
char    incomp_bin[] = "Executable file incompatible with hardware"; /*nl_msg 660 */

#ifndef NLS
struct sysnod commands[] =
{
	{ ".",		SYSDOT	},
	{ ":",		SYSNULL	},
#ifndef RES
	{ "[",		SYSTST },
#endif /* RES */
	{ "break",	SYSBREAK },
	{ "cd",		SYSCD	},
	{ "continue",	SYSCONT	},
	{ "echo",	SYSECHO },
	{ "eval",	SYSEVAL	},
	{ "exec",	SYSEXEC	},
	{ "exit",	SYSEXIT	},
	{ "export",	SYSXPORT },
	{ "hash",	SYSHASH	},

#ifdef RES
	{ "login",	SYSLOGIN },
#endif /* RES */
#if defined(SYSNETUNAM) && defined(RFA)
	{ "netunam",	SYSNETUNAM },
#endif /* SYSNETUNAM && RFA */
#ifdef RES
	{ "newgrp",	SYSLOGIN },
#else /* RES */
	{ "newgrp",	SYSNEWGRP },
#endif /* RES */

	{ "pwd",	SYSPWD },
	{ "read",	SYSREAD	},
	{ "readonly",	SYSRDONLY },
	{ "return",	SYSRETURN },
	{ "set",	SYSSET	},
	{ "shift",	SYSSHFT	},
	{ "test",	SYSTST },
	{ "times",	SYSTIMES },
	{ "trap",	SYSTRAP	},
	{ "type",	SYSTYPE },


#ifndef RES
	{ "ulimit",	SYSULIMIT },
	{ "umask",	SYSUMASK },
#endif /* RES */

	{ "unset", 	SYSUNS },
	{ "wait",	SYSWAIT	}
};
#else /* NLS */
struct sysnod commands[] =
{
	(tchar *)"\0.\0",			SYSDOT,
	(tchar *)"\0:\0",			SYSNULL,
#ifndef RES
	(tchar *)"\0[\0",			SYSTST,
#endif /* RES */
	(tchar *)"\0b\0r\0e\0a\0k\0",		SYSBREAK,
	(tchar *)"\0c\0d\0",			SYSCD,
	(tchar *)"\0c\0o\0n\0t\0i\0n\0u\0e\0",	SYSCONT,
	(tchar *)"\0e\0c\0h\0o\0",		SYSECHO,
	(tchar *)"\0e\0v\0a\0l\0",		SYSEVAL,
	(tchar *)"\0e\0x\0e\0c\0",		SYSEXEC,
	(tchar *)"\0e\0x\0i\0t\0",		SYSEXIT,
	(tchar *)"\0e\0x\0p\0o\0r\0t\0",	SYSXPORT,
	(tchar *)"\0h\0a\0s\0h\0",		SYSHASH,
#ifdef RES
	(tchar *)"\0l\0o\0g\0i\0n\0",		SYSLOGIN,
#endif /* RES */
#if defined(SYSNETUNAM) && defined(RFA)
	(tchar *)"\0n\0e\0t\0u\0n\0a\0m\0",	SYSNETUNAM,
#endif /* SYSNETUNAM && RFA */
#ifdef RES
	(tchar *)"\0n\0e\0w\0g\0r\0p\0",	SYSLOGIN,
#else /* RES */
	(tchar *)"\0n\0e\0w\0g\0r\0p\0",	SYSNEWGRP,
#endif /* RES */
	(tchar *)"\0p\0w\0d\0",			SYSPWD,
	(tchar *)"\0r\0e\0a\0d\0",		SYSREAD,
	(tchar *)"\0r\0e\0a\0d\0o\0n\0l\0y\0",	SYSRDONLY,
	(tchar *)"\0r\0e\0t\0u\0r\0n\0",	SYSRETURN,
	(tchar *)"\0s\0e\0t\0",			SYSSET,
	(tchar *)"\0s\0h\0i\0f\0t\0",		SYSSHFT,
	(tchar *)"\0t\0e\0s\0t\0",		SYSTST,
	(tchar *)"\0t\0i\0m\0e\0s\0",		SYSTIMES,
	(tchar *)"\0t\0r\0a\0p\0",		SYSTRAP,
	(tchar *)"\0t\0y\0p\0e\0",		SYSTYPE,
#ifndef RES
	(tchar *)"\0u\0l\0i\0m\0i\0t\0",	SYSULIMIT,
	(tchar *)"\0u\0m\0a\0s\0k\0",		SYSUMASK,
#endif /* RES */
	(tchar *)"\0u\0n\0s\0e\0t\0", 		SYSUNS,
	(tchar *)"\0w\0a\0i\0t\0",		SYSWAIT
};
#endif /* NLS */

char *nl_msg(i,s)
int i;
char *s;
{
#ifdef NLS
   char *msgstr;
   nl_catd nl_fd;

   nl_fd = catopen(langpath,0);
   msgstr = catgets(nl_fd,NL_SETN,i,s);
   catclose(nl_fd);
   return(msgstr);
#else
   return(s);
#endif
}

int no_commands = sizeof(commands)/sizeof(struct sysnod);
