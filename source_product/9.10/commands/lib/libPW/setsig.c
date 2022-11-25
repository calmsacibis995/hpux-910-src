/* @(#) $Revision: 37.2 $ */      
# include	"signal.h"
# include	"sys/types.h"
# include	"macros.h"

#define ONSIG	16

/*
	General-purpose signal setting routine.
	All non-ignored, non-caught signals are caught.
	If a signal other than hangup, interrupt, or quit is caught,
	a "user-oriented" message is printed on file descriptor 2 with
	a number for help(I).
	If hangup, interrupt or quit is caught, that signal	
	is set to ignore.
	Termination is like that of "fatal",
	via "clean_up(sig)" (sig is the signal number)
	and "exit(userexit(1))".
 
	If the file "dump.core" exists in the current directory
	the function commits
	suicide to produce a core dump
	(after calling clean_up, but before calling userexit).
*/


char	*Mesg[ONSIG]={
	0,
	0,	/* Hangup */
	0,	/* Interrupt */
	0,	/* Quit */
	"Illegal instruction",
	"Trace/BPT trap",
	"IOT trap",
	"EMT trap",
	"Floating exception",
	"Killed",
	"Bus error",
	"Memory fault",
	"Bad system call",
	"Broken pipe",
	"Alarm clock"
};

extern int setsig1();
#ifdef	hpux
static struct sigvec vec = { setsig1, 0, 0 };
#endif	hpux
#ifdef	hp9000s500
#define	OLD_BELL_SIG -1
#endif	hp9000s500

setsig()
{
	extern int setsig1();
	register int j, n;
	struct sigvec ovec;

	for (j=1; j<ONSIG; j++) {
#ifndef	hpux
		if (n=(int)signal(j,setsig1))
			signal(j,n);
#else	hpux
		(void) sigvector(j, &vec, &ovec);
		if (ovec.sv_handler) {
# ifndef hp9000s500
			(void) sigvector(j, &ovec, (struct sigvec *)0);
# else hp9000s500
			if	(ovec.sv_mask == OLD_BELL_SIG)
				signal(j,ovec.sv_handler);
			else	(void) sigvector(j, &ovec, (struct sigvec *)0);
# endif hp9000s500
		}
#endif	hpux
	}
}


static char preface[]="SIGNAL: ";
static char endmsg[]=" (ut12)\n";

setsig1(sig)
int sig;
{

	if (Mesg[sig]) {
		write(2,preface,length(preface));
		write(2,Mesg[sig],length(Mesg[sig]));
		write(2,endmsg,length(endmsg));
	}
	else
		signal(sig,SIG_IGN);
	clean_up(sig);
	if(open("dump.core",0) > 0) {
		signal(SIGIOT,SIG_DFL);
		abort();
	}
	exit(userexit(1));
}
