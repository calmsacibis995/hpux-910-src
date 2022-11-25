/* HPUX_ID: @(#) $Revision: 72.1 $ */
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
 *  Job control for UNIX Shell
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 3C-526B
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *
 *  Written October, 1982
 *  Rewritten April, 1988
 */


#include	<errno.h>
#include	"defs.h"
#include	"jobs.h"
#include	"history.h"

#ifdef _sys_wait_
#  include	<sys/wait.h>
#  undef _wait_
#  define _wait_ 1
#else
#  ifdef _wait_
#     include	<wait.h>
#  endif /* _wait_*/
#endif	/* _sys_wait_ */

#ifdef _sys_sigaction_
#  include	<sys/sigaction.h>
#endif /* _sys_sigaction_ */

#ifdef WAIT1ARG
#   undef WNOHANG
#endif /* WAIT1ARG */

#undef	WIFSTOPPED
#define WIFSTOPPED(w)		(((w)&LOBYTE)==0177)
#undef	WSTOPSIG
#define WSTOPSIG(w)		(((w)>>8)&LOBYTE)
#undef	WIFKILLED
#define WIFKILLED(w)		((w)&LOBYTE)
#undef	WKILLSIG
#define WKILLSIG(w)		((w)&(LOBYTE&~HIGHBIT))
#undef	WEXITVAL
#define WEXITVAL(w)		((((unsigned)(w))>>8)&LOBYTE)
#undef	WIFDUMPED
#define WIFDUMPED(w)		(w&HIGHBIT)

static struct process	*job_bypid();
static struct process	*job_byjid();
static char		*job_sigmsg();
static int		job_alloc();
static void 		job_free();
static int		job_unpost();
static void		job_unlink();
static struct process	*freelist;
static char		beenhere;
static struct process	dummy;

#ifdef JOBS
   static void			job_set();
   static void			job_reset();
   static struct process	*job_byname();
   static struct process	*job_bystring();
   static struct termios	my_stty;  /* terminal state for shell */
   static char			*job_string;
#else
#  undef SIGCHLD
   extern const char		e_coredump[];
#endif /* JOBS */

#ifdef SIGTSTP
    static void		job_unstop();
    static void		job_fgrp();
#   ifdef waitpid
#	ifdef TIOCGPGRP
#	   define tcgetpgrp(a) (ioctl(a, TIOCGPGRP, &job.mytgid)>=0?job.mytgid:-1)	
#	endif /* TIOCGPGRP */
	int tcsetpgrp(fd,pgrp)
	int fd;
	pid_t pgrp;
	{
#	ifdef TIOCGPGRP
		return(ioctl(fd, TIOCSPGRP, &pgrp));	
#	else
		return(-1);
#	endif /* TIOCGPGRP */
	}
#   endif /* waitpid */
#else
#   define job_unstop(pw)
#endif /* SIGTSTP */

#ifndef OTTYDISC
#   undef NTTYDISC
#endif /* OTTYDISC */


#ifdef JOBS
int	maxj = MAXJ;		/* maximum job number */
int	jbytes = JBYTES;	/* Number of bytes to make maxj bits */
#endif /* JOBS */

#ifdef POSIX2
int w_njob = 1;
static struct {
	int pid;
	unsigned short exstat;	/* exit status of background processes */
} w_stat[JBYTES+1];		/* w_stat[0] is a temporary save location */
#endif /*POSIX2 */

#ifdef JOBS
#ifdef SIGTSTP
/*
 * initialize job control
 * if lflag is set the switching driver message will not print
 */

void job_init(lflag)
{
	register int ntry = 0;
	int 	tty_ntry=0;

#ifdef POSIX2
	register int i;
		for(i=0; i<JBYTES+1; i++)
			w_stat[i].pid = 0;
		w_njob = 1;	/* reset job count */
#endif /* POSIX2 */

	/* use new line discipline when available */
#   ifdef NTTYDISC
#     ifdef FIOLOOKLD
	if((job.linedisc = ioctl (jobtty, FIOLOOKLD, 0)) <0)
#     else
	if(ioctl(jobtty,TIOCGETD,&job.linedisc) !=0)
#     endif /* FIOLOOKLD */
		return;
	if(job.linedisc!=NTTYDISC && job.linedisc!=OTTYDISC)
	{
		/* no job control when running with MPX */
#     ifdef VSH
		on_option(VIRAW);
#     endif /* VSH */
		return;
	}
	if(job.linedisc==NTTYDISC)
	{
		job.linedisc = -1;
	}
#   endif /* NTTYDISC */

	if((job.mypgid=getpgid(0))<=0)
		return;

	/* wait until we are in the foreground */
	/* DSDe408307
	 * Needs to check stdin, stdout, and stderr, not just 
	 * JOBTTY (2). When user redirect stderr to a file,
	 * tcgetpgrp() will fail with ENOTTY, so the process group
	 * is not set up correctly. Later on, when the child process
	 * tries to read from the terminal, the user can never type in
	 * anything even though the child process is the foreground 
	 * process. This checking also needs to be done in job_reset(),
	 * to make sure the parent process gets the control terminal 
	 * back.
	 */ 
	while((job.mytgid=tcgetpgrp(jobtty)) != job.mypgid)
	{
		if(job.mytgid == -1) {
			if (errno==ENOTTY && tty_ntry<2) {
				jobtty = ++jobtty%3;
				tty_ntry++;
			}
			else
				return;
		}
		else {
			/* Stop this shell until continued */
			signal(SIGTTIN,SIG_DFL);
			kill(sh.pid,SIGTTIN);
			/* resumes here after continue tries again */
			if(ntry++ > MAXTRY)
			{
				p_str(e_no_start,NL);
				return;
			}
		}
	}

#   ifdef NTTYDISC
	/* set the line discipline */
	if(job.linedisc>=0)
	{
		int linedisc = NTTYDISC;
#	ifdef FIOPUSHLD
		tty_get(jobtty,&my_stty);
		if (ioctl(jobtty, FIOPOPLD, 0) < 0)
			return;
		if (ioctl(jobtty, FIOPUSHLD, &linedisc) < 0)
		{
			ioctl(jobtty, FIOPUSHLD, &job.linedisc);
			return;
		}
		tty_set(jobtty,TCSANOW,&my_stty);
#	else
		if(ioctl(jobtty,TIOCSETD,&linedisc) !=0)
			return;
#	endif /* FIOPUSHLD */
		if(lflag==0)
			p_str(e_newtty,NL);
		else
			job.linedisc = -1;
	}
#   endif /* NTTYDISC */

	/* make sure that we are a process group leader */
	setpgid(0,sh.pid);
#   ifdef SA_NOCLDWAIT
	sigflag(SIGCLD, SA_NOCLDSTOP|SA_NOCLDWAIT, 0);
#   endif /* SA_NOCLDWAIT */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	tcsetpgrp(jobtty,sh.pid);
#   ifdef CNSUSP
	/* set the switch character */
	tty_get(jobtty,&my_stty);
	job.suspend = my_stty.c_cc[VSUSP];
	if(job.suspend == CNSUSP)
	{
		my_stty.c_cc[VSUSP] = CNSUSP;
		tty_set(jobtty,TCSAFLUSH,&my_stty);
	}
#   endif /* CNSUSP */
	/*
	 *  Check to see if we did a set +m in the $ENV file
	 *  and if so, we don't want to turn on monitor. See
	 *  also main.c:main() and args.c:arg_opts().
	 */
	if(env_monitor != OFF)
		on_option(MONITOR);
	job.jobcontrol++;
	return;
}


/*
 * see if there are any stopped jobs
 * restore tty driver and pgrp
 */

int job_close()
{
	register struct process *pw = job.pwlist;
	register int count = 0;
	if(!job.jobcontrol)
		return(0);
	for(;pw;pw=pw->p_nxtjob)
	{
		if(!(pw->p_flag&P_STOPPED))
			continue;
		if(beenhere)
			killpg(pw->p_pgrp,SIGTERM);
		count++;
	}
	if(beenhere++ == 0 && job.pwlist)
	{
		p_setout(ERRIO);
		if(count)
		{
			p_str(e_terminate,NL);
			return(-1);
		}
		else if(sh.login_sh)
		{
			p_str(e_running,NL);
			return(-1);
		}
	}
	setpgid(0,job.mypgid);
	tcsetpgrp(jobtty,job.mypgid);
#   ifdef NTTYDISC
	if(job.linedisc>=0)
	{
		/* restore old line discipline */
#	ifdef FIOPUSHLD
		tty_get(jobtty,&my_stty);
		if (ioctl(jobtty, FIOPOPLD, 0) < 0)
			return(0);
		if (ioctl(jobtty, FIOPUSHLD, &job.linedisc) < 0)
		{
			job.linedisc = NTTYDISC;
			ioctl(jobtty, FIOPUSHLD, &job.linedisc);
			return(0);
		}
		tty_set(jobtty,TCSAFLUSH,&my_stty);
#	else
		if(ioctl(jobtty,TIOCSETD,&job.linedisc) !=0)
			return(0);
#	endif /* FIOPUSHLD */
		p_str(e_oldtty,NL);
	}
#   endif /* NTTYDISC */
#   ifdef CNSUSP
	if(job.suspend==CNSUSP)
	{
		tty_get(jobtty,&my_stty);
		my_stty.c_cc[VSUSP] = CNSUSP;
		tty_set(jobtty,TCSAFLUSH,&my_stty);
	}
#   endif /* CNSUSP */
	job.jobcontrol = 0;
	return(0);
}


/*
 * Set the ttygroup id to a previously running job
 */
#else
void job_init(flag)
{
}

int job_close()
{
	if(!(st.states&MONITOR))
		return(0);
	if(beenhere++ == 0 && job.pwlist && sh.login_sh)
	{
		p_str(e_running,NL);
		return(-1);
	}
	return(0);
}
#endif	/* SIGTSTP */

static void job_set(pw)
register struct process *pw;
{
	/* save current terminal state */
	tty_get(jobtty,&my_stty);
	if(pw->p_flag&P_STTY)
	{
		/* restore terminal state for job */
		tty_set(jobtty,TCSAFLUSH,&pw->p_stty);
	}
#ifdef SIGTSTP
	if(tcsetpgrp(jobtty,pw->p_fgrp) !=0  && (errno == EPERM))
	    tcsetpgrp(jobtty,pw->p_pgrp);
	/* if job is stopped, resume it in the background */
	job_unstop(pw);
#endif	/* SIGTSTP */
}

static void	job_reset(pw)
register struct process *pw;
{
	/* save the terminal state for current job */
#ifdef SIGTSTP
	job_fgrp(pw,tcgetpgrp(jobtty));
	if(tcsetpgrp(jobtty,sh.pid) !=0) {
		jobtty = ++jobtty%3;
		if (tcsetpgrp(jobtty,sh.pid) == -1) {
			jobtty = ++jobtty%3;
			if (tcsetpgrp(jobtty,sh.pid) == -1)
				return;
		}
	}
#endif	/* SIGTSTP */
	if(pw && (pw->p_flag&P_SIGNALLED))
	{
		if(tty_get(jobtty,&pw->p_stty) == 0)
			pw->p_flag |= P_STTY;
		/* restore terminal state for job */
		tty_set(jobtty,TCSAFLUSH,&my_stty);
	}
	beenhere = 0;
}
#endif /* JOBS */

/*
 * wait built-in command
 */

void job_bwait(jobs)
char *jobs[];
{
	register char *job;
	register struct process *pw;
	register pid_t pid;
	register int i;
	char *ptr = e_nullstr;
	if(*jobs==0) {
		job_wait((pid_t)-1);
		sh.exitval = 0;
#ifdef POSIX2
		for(i=0; i<w_njob; i++)
			w_stat[i].pid = 0;
		w_njob = 1;	/* reset job count */
#endif /* POSIX2 */
	}
	else while(job = *jobs++)
	{
#ifdef JOBS
		if(*job == '%')
		{
			if(pw = job_bystring(job))
				pid = pw->p_pid;
			else {
				sh.exitval = 1;	/* error in name */
				return;
			}
		}
		else
#endif /* JOBS */
                {
                        int eflg;
                        pid = string_to_long(job, 10, &eflg);
			if(eflg) {
				sh.exitval = 127;	/* bad number */
				continue;
			}
                }
			i = 1;		/* set for both for loops that follow */
		if(job_wait(-pid) == -1) {
			sh.exitval = 127;	/* bogus job, set exit value
						 * 127 is a POSIX requirement */
#ifdef POSIX2
			/* if the job was already waited for, and we had saved the
			 * return value, return that saved value now
			 */
			for(; i<w_njob; i++)
				if(w_stat[i].pid == pid)  /* found the job */
					sh.exitval = w_stat[i].exstat;
				i--;	/* points to the job entry */
#endif /* POSIX2 */
		}
#ifdef POSIX2
		/* delete entry if found */
		for(; i<w_njob; i++)
			if(w_stat[i].pid == pid)  /* found the job */
			{
				/* remove entry and compact array */
				for(; i<w_njob-1; i++)
					w_stat[i] = w_stat[i+1];
				w_stat[--w_njob].pid = 0;
			}
#endif /* POSIX2 */
	}
}

#ifdef JOBS
/*
 * execute function <fun> for each job
 */

int job_walk(fun,arg,jobs)
int (*fun)();
char *jobs[];
{
	register struct process *pw = job.pwlist;
	register int r = 0;
	register char *job;
	register struct process *px;
	job_string = 0;
	if(jobs==0)
	{
		/* do all jobs */
		for(;pw;pw=px)
		{
			px = pw->p_nxtjob;
			if((*fun)(pw,arg))
				r = 2;
		}
	}
	else if(*jobs==0)	/* current job */
	{
		/* skip over non-stop jobs */
		while(pw && pw->p_pgrp==0)
			pw = pw->p_nxtjob;
		if((*fun)(pw,arg))
			r = 2;
	}
	else while(job = *jobs++)
	{
		job_string = job;
		if(*job==0)
			sh_fail(job_string,e_jobusage);
		if(*job == '%')
			pw = job_bystring(job);
		else
		{
			int pid;
			int eflg;

			/*
			 *  We use strtol instead of atoi
			 *  so that we can do error checking.
			 *  If atoi gets an error and returns 0
			 *  we could kill the wrong processes.
			 */
                        pid = string_to_long(job, 10, &eflg);
			if(eflg)
			    sh_fail(job_string,e_jobusage);

			if(pid<0)
				job++;
			while(isdigit(*job))
				job++;
			if(*job)
				sh_fail(job_string,e_jobusage);
			if(!(pw = job_bypid(pid)))
			{
				pw = &dummy;
				pw->p_pid = pid;
				pw->p_pgrp = pid;
			}
			pw->p_flag |= P_BYNUM;
		}
		if((*fun)(pw,arg))
			r = 2;
		if(pw)
			pw->p_flag &= ~P_BYNUM;
	}
	return(r);
}

/*
 * list the given job
 * flag L_FLAG for long listing
 * flag N_FLAG for list only jobs marked for notification
 * flag P_FLAG for process id(s) only
 */

int job_list(pw,flag)
struct process *pw;
register int flag;
{
	register struct process *px = pw;
	register int  n;
	register const char *msg;
	static char m[2];
	int msize;
	if(pw==0)
		return(1);
	if(job.pwlist==NULL)
		return(0);
	if(job.waitsafe)
		job_wait((pid_t)0);
	if((flag&N_FLAG) && (!(px->p_flag&P_NOTIFY)||px->p_pgrp==0))
		return(0);
	p_setout(st.standout);
	if((flag&P_FLAG))
	{
		p_num(px->p_pgrp,NL);
		return(0);
	}
	n = px->p_job;
	p_sub(n,' ');
	if(px==job.pwlist)
		m[0] = '+';
	else if(px==job.pwlist->p_nxtjob)
		m[0] = '-';
	else
		m[0] = ' ';
	p_str(m,' ');
	do
	{
		n = 0;
		if(flag&L_FLAG)
			p_num(px->p_pid,'\t');
		if(px->p_flag&P_SIGNALLED)
			msg = job_sigmsg((int)(px->p_exit));
		else if(px->p_flag&P_NOTIFY)
		{
			msg = e_Done;
			n = px->p_exit;
		}
		else
			msg = e_Running;
		p_str(msg,0);
		msize = strlen(msg);
		if(n)
		{
			p_str(e_nullstr,'(');
			p_num((int)n,')');
			msize += (3+(n>10)+(n>100));
		}
		if(px->p_flag&P_COREDUMP)
		{
			p_str(e_coredump,0);
			msize += strlen(e_coredump);
		}
		p_nchr(SP,MAXMSG-msize);
		if(flag&L_FLAG)
			px = px->p_nxtproc;
		else
			px = NULL;
		if(px==NULL)
			hist_list(pw->p_name,0,";");
		else
			p_str(e_nlspace,0);
	}
	while(px);
	px = pw;
	if(px->p_flag&P_STOPPED)
		px->p_flag &= ~P_NOTIFY;
	else if(px->p_flag&P_DONE)
	{
		if(!job_unpost(px))
			px->p_flag &= ~P_NOTIFY;
	}
	return(0);
}

/*
 * get the process group given the job number
 * This routine returns the process group number or -1
 */

static struct process *job_bystring(ajob)
register char *ajob;
{
	register struct process *pw=job.pwlist;
	register int c;
	if(*ajob++ != '%' || pw==NULL)
		return(NULL);
	c = *ajob;
	if(isdigit(c))
		pw = job_byjid(atoi(ajob));
	else if(c=='+' || c=='%')
		;
	else if(c=='-')
	{
		if(pw)
			pw = job.pwlist->p_nxtjob;
	}
	else
		pw = job_byname(ajob);
	if(pw && pw->p_flag)
		return(pw);
	return(NULL);
}

/*
 * Kill a job or process
 */

int job_kill(pw,sig)
register struct process *pw;
int sig;
{
	register pid_t pid;
	register int r;
	const char *msg;
#ifdef SIGTSTP
	int stopsig = (sig == SIGSTOP || sig == SIGTSTP || sig == SIGTTOU || sig == SIGTTIN);
        int contsig = (sig == SIGCONT);
#else
#	define stopsig 1
#       define contsig 1
#endif  /* SIGTSTP */
	errno = 0;
	if(pw==0)
		goto error;
	pid = pw->p_pid;
	if(pw->p_flag&P_BYNUM)
	{
#ifdef SIGTSTP
  		if(sig==SIGSTOP && pid==sh.pid && sh.ppid==1) 
/*		if(sig==SIGSTOP && pid==sh.pid && *sh.shname=='-') 
by tkr */
		{
			/* can't stop login shell */
			errno = EPERM;
			r = -1;
		}
		else
		{
			if(pw->p_flag&P_STOPPED)
				pw->p_flag &= ~(P_STOPPED|P_SIGNALLED);
			if(pid>=0)
			{
				/*
				 *  Need to send the signal first, then
				 *  send a SIGCONT incase the job is stopped.
				 *  We don't send the SIGCONT if the action of
				 *  sig is to stop the process.
				 *  Added additional check for SIGCONT.
                                 *  Now send SIGCONT only once if one is
                                 *  trying to SIGCONT a process.
                                 *  Sending SIGCONT twice broke scripts
                                 *  See UCSqm00366.
				 */
				if((r = kill(pid,sig)) >= 0 && !stopsig && !contsig)
				    kill(pid,SIGCONT);
			}
			else
			{
				/*
				 *  we use -pid here with killpg because
				 *  pid is already negative.
				 */
				if((r = killpg(-pid,sig)) >= 0 && !stopsig && !contsig)
				    killpg(-pid,SIGCONT);
			}
		}
#else
		if(pid>=0)
			r = kill(pid,sig);
		else
			r = killpg(-pid,sig);
#endif	/* SIGTSTP */
	}
	else
	{
		if(pid = pw->p_pgrp)
		{
			if((r = killpg(pid,sig)) >= 0 && !stopsig)
			    job_unstop(pw);
			if(r>=0)
				time((time_t)0); /* delay a little for SIGCLD */
		}
		while(pw && pw->p_pgrp==0 && (r=kill(pw->p_pid,sig))>=0) 
		{
#ifdef SIGTSTP
			if(!stopsig)
			    kill(pw->p_pid,SIGCONT);
#endif  /* SIGTSTP */
			pw = pw->p_nxtproc;
		}
	}
	if(r<0 && job_string)
	{
	error:
		p_setout(ERRIO);
		p_str(e_killcolon,0);
		if(pw && pw->p_flag&P_BYNUM)
			msg = e_no_proc;
		else
			msg = e_no_job;
		p_str(job_string,0);
		p_str(e_colon,0);
		if(errno == EPERM)
			msg = e_access;
		p_str(msg,NL);
		r = 2;
	}
	return(r);
}

/*
 * Get process structure from first letters of jobname
 *
 */

static struct process *job_byname(name)
char *name;
{
	register struct process *pw = job.pwlist;
	register struct process *pz = 0;
	register int flag = 0;
	register char *cp = name;
	if(hist_ptr==NULL)
		return(NULL);
	if(*cp=='?')
		(cp++,flag++);
	for(;pw;pw=pw->p_nxtjob)
	{
		if(hist_match(pw->p_name,cp,flag)>=0)
		{
			if(pz)
				sh_fail(name-1,e_ambiguous);
			pz = pw;
		}
	}
	return(pz);
}

#else
#   define job_set(x)
#   define job_reset(x)
#endif /* JOBS */



/*
 * Initialize the process posting array
 */

void	job_clear()
{
	register struct process *pw, *px;
	register struct process *pwnext;
	register int j = JBYTES;
	for(pw=job.pwlist; pw; pw=pwnext)
	{
		pwnext = pw->p_nxtjob;
		for(px=pw; px; px=px->p_nxtproc)
			free((char*)px);
	}
	job.pwlist = NULL;
	job.numpost=0;
#ifdef JOBS
	job.jobcontrol = 0;
#endif /* JOBS */
	while(--j >=0)
		job.freejobs[j]  = 0;
}

/*
 * put the process <pid> on the process list and return the job number
 */

int job_post(pid)
pid_t pid;
{
	register struct process *pw;
	register struct history *fp = hist_ptr;

	if(pw=freelist)
		freelist = pw->p_nxtjob;
	else
		pw = new_of(struct process,0);
	job.numpost++;
	if(job.pipeflag && job.pwlist)
	{
		/* join existing current job */
		pw->p_nxtjob = job.pwlist->p_nxtjob;
		pw->p_nxtproc = job.pwlist;
		pw->p_job = job.pwlist->p_job;
	}
	else
	{
		/* create a new job */
		pw->p_nxtjob = job.pwlist;
		pw->p_job = job_alloc();
		pw->p_nxtproc = 0;
	}
	job.pwlist = pw;
	pw->p_pid = pid;
	pw->p_flag = P_RUNNING;
	if(st.states&MONITOR)
		pw->p_fgrp = job.curpgid;
	else
		pw->p_fgrp = 0;
	pw->p_pgrp = pw->p_fgrp;
#ifdef POSIX2
	w_stat[0].pid = pid;		/* temporarily save pid */
#endif /* POSIX2 */
#ifdef DBUG
	p_setout(ERRIO);
	p_num(getpid(),':');
	p_str("posting",' ');
	p_num(pw->p_job,' ');
	p_str("pid",'=');
	p_num(pw->p_pid,' ');
	p_str("pgid",'=');
	p_num(pw->p_pgrp,NL);
	p_flush();
#endif /* DBUG */
#ifdef JOBS
	if(fp!=NULL)
		pw->p_name=hist_position(fp->fixind-1);
	else
		pw->p_name = -1;
#endif /* JOBS */
	if(job.numpost >= maxj-1)
	{
	   	unsigned char *x;
		int i;
		
		x = (unsigned char *)malloc(2*jbytes);

		/* copy the current job  bit vector */
		for(i=0; i<jbytes; i++)
		    x[i]=job.freejobs[i];

		jbytes *= 2;
		maxj *= 2;
		/* malloc doesn't zero out the space */
		for(/* i is ok */; i<jbytes; i++)
		    x[i]=0;

		free(job.freejobs);
		job.freejobs = x;
	}
	return(pw->p_job);
}

/*
 * Returns a process structure give a process id
 */

static struct process *job_bypid(pid)
pid_t 	pid;
{
	register struct process  *pw, *px;
	for(pw=job.pwlist; pw; pw=pw->p_nxtjob)
		for(px=pw; px; px=px->p_nxtproc)
		{
			if(px->p_pid==pid)
				return(px);
		}
	return(NULL);
}

/*
 * return a pointer to a job given the job id
 */

static struct process *job_byjid(jobid)
{
	register struct process *pw;
	for(pw=job.pwlist;pw; pw = pw->p_nxtjob)
	{
		if(pw->p_job==jobid)
			break;
	}
	return(pw);
}

#ifndef SIG_NORESTART
static jmp_buf waitintr;
static VOID interrupt()
{
	st.intfn = 0;
	longjmp(waitintr,1);
}
#endif /* SIG_NORESTART */

/*
 * Wait for process pid to complete
 * If pid < -1, then wait can be interrupted, -pid is waited for
 * pid=0 to wait for any process
 * pid=1 to wait for at least one process to complete
 * pid=-1 to wait for all runing processes
 *
 * job_wait returns -1 if it can't find a specified job.
 * this is so that job_bwait can detect a bogus job specification
 * and print the appropriate error.
 */

job_wait(pid)
register pid_t 	pid;
{
	register struct process *pw = NULL;
	register int 	wstat;
	register pid_t	myjob;
	register pid_t 	p;
	int		jobid = 0;
	int		w;
#ifdef WUNTRACED
	int		waitflag = WUNTRACED;
#endif /* WUNTRACED */
	char		intr = 0;
	char		bgjob = 0;
	pid_t		wait_pid = -1;

	/*
	 *  When doing a wait on the process forked
	 *  for command substitution, we do NOT want
	 *  to reap any extra processes.  We might reap
	 *  the last process in the process group of the
	 *  current pipeline and then subsequent stpgid()'s
	 *  would fail leaving the rest of the pipeline in
	 *  the background (only an issue for command subs
	 *  in the last element of a pipeline).
	 */
	if(hp_flags1&HPCOMSUB)
	{
		/*
		 *  If we are not waiting for
		 *  the command susbtitution 
		 *  process, defer until later
		 */
		if (pid != sh.subpid)
		    	return(0);
		wait_pid = sh.subpid;
	}
	
	if(pid==0)
	{
		if(!job.waitsafe)
#ifdef WNOHANG
  			waitflag |= WNOHANG;
#else
			return(0);
#endif  /* WNOHANG */
	}

	if(pid < 0)
	{
		pid = -pid;
		intr = 1;
#ifndef SIG_NORESTART
		st.intfn = interrupt;
		if(setjmp(waitintr))
			goto done;
#endif /* SIG_NORESTART */
	}
	if(pid > 1)
	{
		if((pw=job_bypid(pid))==NULL)
			return(-1);
		jobid = pw->p_job;
		if(intr==0 && pw->p_pgrp)
			job_set(job_byjid(jobid));
		if(pw->p_flag&P_DONE)
			goto jobdone;
	}
#ifdef DBUG
	p_setout(ERRIO);
	p_num(getpid(),':');
	p_str(" job_wait job",'=');
	p_num(jobid,' ');
	p_str("pid",'=');
	p_num(pid,NL);
#endif /* DBUG*/
	p_flush();
#ifdef SIGCHLD
	signal(SIGCHLD, SIG_DFL);
	job.waitsafe = 0;
#endif /* SIGCHLD */
	while(1)
	{
		if((p=waitpid(wait_pid,&w,waitflag))== -1)
		{
			if(errno==EINTR &&!intr)
				continue;
			goto done;
		}
		if(p==0)
			goto done;
		if((pw = job_bypid(p))==0)
#ifdef JOBS
			goto tryagain;
#else
			continue;
#endif /* JOBS */
		wstat = w;
		myjob= (pw->p_job==jobid);
		if(!myjob && pw->p_pgrp && pw->p_pgrp!=job.curpgid)
			bgjob = 1;
		pw->p_flag &= ~P_RUNNING;
#ifdef POSIX2
		sav_wstat(p, w);
#endif /* POSIX2 */
#ifdef DBUG
		p_num(getpid(),':');
		p_str(" job with pid",' ');
		p_num(p,' ');
		p_str("complete with status",'=');
		p_num(w,' ');
		p_str("jobid",'=');
		p_num(pw->p_job,NL);
		p_flush();
#endif /* DBUG*/
#ifdef SIGTSTP
		if (WIFSTOPPED(wstat))
		{
			pw->p_exit = WSTOPSIG(wstat);
#ifdef POSIX2
			sav_wstat(p, 128+pw->p_exit);
#endif /* POSIX2 */
			if(myjob && (pw->p_exit==SIGTTIN||pw->p_exit==SIGTTOU) &&
			   !(pw->p_flag&P_RESTARTED))
			{
				/* job stopped prematurely, restart it */
				killpg(pw->p_pgrp,SIGCONT);
				/* 
				 *  Don't want to restart a job more than once
				 *  since we only reset the tty group once.
				 */
				pw->p_flag |= P_RESTARTED;
				continue;
			}
			pw->p_flag |= (P_NOTIFY|P_SIGNALLED|P_STOPPED);
			if(myjob)
			{
				if((pw=job_byjid(jobid)) && (pw->p_flag&P_RUNNING))
						goto tryagain;

                                /* SIGSTOP to child of non-interactive
                                 * ksh kills ksh.
				 * UCSqm00448
				 * Test for: if I'm not interactive (i.e., I
				 * didn't set up job control) then, I don't
				 * know what to do with a stopped child, so
				 * just ignore it.
                                 */
                                if (pid && !is_option(MONITOR))
				    continue;  /* call waitpid again */
                                else
				    job_wait((pid_t)0);
				goto done;
			}
			goto tryagain;
		}
		else
#endif /* SIGTSTP */
		{
			if(p==sh.cpid)
			{
				io_fclose(COTPIPE);
				io_fclose(sh.cpipe[1]);
				sh.cpipe[1] = -1;
			}
			if (WIFKILLED(wstat))
			{
				pw->p_flag |= (P_DONE|P_SIGNALLED);
				if (WIFDUMPED(wstat))
					pw->p_flag |= P_COREDUMP;
				pw->p_exit = WKILLSIG(wstat);
#ifdef POSIX2
				sav_wstat(p, 128+pw->p_exit);
#endif /* POSIX2 */
				if(!myjob)
					pw->p_flag |= P_NOTIFY;
				/*
				 *  If we have job control, pass the SIGINT
				 *  information back to parent so we can do
				 *  a trap on INT in the parent.  But don't
				 *  kill the shell if we are a shell script.
				 *  This is the way the old (ksh-i) did it.
				 */
				else if(st.states&MONITOR && pw->p_exit==SIGINT)
					sh_fault(SIGINT); 
#ifdef JOBS
				if(myjob)
#endif /* JOBS */
				{
					p_setout(ERRIO);
					if(pw->p_exit!=SIGINT &&
						pw->p_exit!=SIGPIPE)
					{
						register char *msg;
						if(pid!=p||!(st.states&PROMPT))
						{
							p_prp(sh_itos(p));
							p_str(e_nullstr,SP);
						}
						if(pw->p_flag&P_COREDUMP)
							p = 0;
						else
							p = NL;
						msg = job_sigmsg((int)(pw->p_exit));
						p_str(msg,p);
						if(p==0)
							p_str(e_coredump,NL);
					}
				}
			}
			else
			{
				pw->p_flag |= (P_DONE|P_NOTIFY);
				pw->p_exit = WEXITVAL(wstat);
#ifdef POSIX2
				sav_wstat(p, pw->p_exit);
#endif /* POSIX2 */
			}
		}
		if(myjob)
		{
		jobdone:
			sh.exitval = job.pwlist->p_exit;
			if(job.pwlist->p_flag&P_SIGNALLED)
				sh.exitval |= SIGFAIL;
			if(job_unpost(pw)) /* job is complete */
				break;
			else if(intr || (job.pwlist==pw && (!(st.states&MONITOR))))
			{
				sh.exitval = pw->p_exit;
				break;
			}
			/* FSDlj09539 - The first element of the pipe
			 * was going to a wait state when the last element
			 * was another command executing in a child
			 * process. The following else will fix the same 

			else if (st.states&LASTPIPE)
				break;                     
			*/ 

			/* The above changes are removed as it
			 * broke some other scripts 
			 */
			continue;
		}
		else if((pw->p_pgrp==0 && p!=sh.subpid) || (pw->p_flag&P_STOPPED))
			job_unpost(pw);
#ifdef JOBS
	tryagain:
		if(pid==0)
		{
#   ifdef WNOHANG
			waitflag |= WNOHANG;
#   else
			signal(SIGCHLD,sh_fault);
			if(!job.waitsafe)
				goto done;
			job.waitsafe = 0;
#   endif /* WNOHANG */
		}
#endif /* JOBS */
	}
	exitset();
done:
	sh.trapnote &= ~SIGSLOW;
#ifdef SIGCHLD
	if(bgjob && st.trapcom[SIGCHLD])
	{
		st.trapflg[SIGCHLD] |= TRAPSET;
		sh.trapnote |= TRAPSET;
	}
#endif /* SIGCHLD */
#ifndef SIG_NORESTART
	st.intfn = 0;
#endif /* SIG_NORESTART */
#ifdef SIGCHLD
	signal(SIGCHLD,sh_fault);
#endif /* SIGCHLD */
	if(pid>1 && intr==0 && pw->p_pgrp)
		job_reset(pw);
		return(0);
}

#ifdef SIGTSTP


/*
 * move job to foreground if bgflag == 0
 * move job to background if bgflag != 0
 */

job_switch(pw,bgflag)
register struct process *pw;
{
	register const char *msg;
	if(!pw || !(pw=job_byjid((int)pw->p_job)))
		return(1);
	p_setout(st.standout);
	if(bgflag)
	{
		p_sub((int)pw->p_job,'\t');
		msg = "&";
	}
	else
	{
		job_unlink(pw);
		pw->p_nxtjob = job.pwlist;
		job.pwlist = pw;
		msg = e_nullstr;
	}
	hist_list(pw->p_name,'&',";");
	p_str(msg,NL);
	p_flush();
	if(!bgflag)
		job_wait(pw->p_pid);
	else if(pw->p_flag&P_STOPPED)
		job_unstop(pw);
	return(0);
}


/*
 * Set the foreground group associated with a job
 */

static void job_fgrp(pw,new)
register struct process *pw;
register int new;
{
	for(; pw; pw=pw->p_nxtproc)
		pw->p_fgrp = new;
}

/*
 * turn off STOP state of a process group and send CONT signals
 */

static void job_unstop(px)
register struct process *px;
{
	register struct process *pw;
	register int num = 0;
	for(pw=px ;pw ;pw=pw->p_nxtproc)
	{
		if(pw->p_flag&P_STOPPED)
		{
			num++;
			pw->p_flag &= ~(P_STOPPED|P_SIGNALLED|P_NOTIFY);
		}
	}
	if(num!=0)
	{
		if(px->p_fgrp != px->p_pgrp)
			killpg(px->p_fgrp,SIGCONT);
		killpg(px->p_pgrp,SIGCONT);
	}
}
#endif	/* SIGTSTP */

/*
 * remove a job from table
 * If all the processes have not completed then unpost returns 0
 * Otherwise the job is removed and unpost returns 1.
 * pwlist is reset if the first job is removed
 */

static int job_unpost(pwtop)
register struct process *pwtop;
{
	register struct process *pw;
	/* make sure all processes are done */
#ifdef DBUG
	p_setout(ERRIO);
	p_num(getpid(),':');
	p_str(" unpost pid",'=');
	p_num(pwtop->p_pid,NL);
	p_flush();
#endif /* DBUG */
	pwtop = pw = job_byjid((int)pwtop->p_job);
	for(; pw && (pw->p_flag&P_DONE); pw=pw->p_nxtproc);
	if(pw)
		return(0);
	/* all processes complete, unpost job */
	job_unlink(pwtop);
	for(pw=pwtop; pw ; pw=pw->p_nxtproc)
	{
		pw->p_flag &= ~P_DONE;
		job.numpost--;
		pw->p_nxtjob = freelist;
		freelist = pw;
	}
#ifdef DBUG
	p_str("free job",'=');
	p_num(pwtop->p_job,NL);
	p_flush();
#endif /* DBUG */
	job_free((int)pwtop->p_job);
	return(1);
}

/*
 * unlink a job form the job list
 */

static void job_unlink(pw)
register struct process *pw;
{
	register struct process *px;
	if(pw==job.pwlist)
	{
		job.pwlist = pw->p_nxtjob;
		return;
	}
	for(px=job.pwlist;px;px=px->p_nxtjob)
		if(px->p_nxtjob == pw)
		{
			px->p_nxtjob = pw->p_nxtjob;
			return;
		}
}

/*
 * get an unused job number
 * freejobs is a bit vector, 0 is unused
 */

static int job_alloc()
{
	register int j=0;
	register unsigned mask = 1;
	register unsigned char *freeword;
	/* skip to first word with a free slot */
	while(job.freejobs[j] == 0xff) 
	{
	    j++;
	    if (j==jbytes)
	    {
	   	unsigned char *x, *p, *q;
		int i;

		x = (unsigned char *)malloc(2*jbytes);
		for(i=0; i<jbytes; i++)
		    x[i]=job.freejobs[i];
		jbytes *= 2;
		maxj *= 2;
		for(/* i is ok */; i<jbytes; i++)
		    x[i]=0;
		free(job.freejobs);
		job.freejobs = x;
	    }
	}
	freeword = &job.freejobs[j];
	j *= 8;
	for(j++;mask&(*freeword);j++,mask <<=1);
	*freeword  |=mask;
	return(j);
}

/*
 * return a job number
 */

static void job_free(n)
register int n;
{
	register int j = (--n)/8;
	register unsigned mask;
	n -= j*8;
	mask = 1 << n;
	job.freejobs[j]  &= ~mask;
}

static char *job_sigmsg(sig)
int sig;
{
	static char signo[] = "Signal xxx";
	if(sig < (MAXTRAP-1) && sh.sigmsg[sig])
		return(sh.sigmsg[sig]);
	sh_copy(sh_itos(sig),signo+7);
	return(signo);
}

#ifdef POSIX2
/* since $! was referenced, we will need to save this pid and its
 * status more permanently
 */
void
chk_wstat(p)
int p;
{
	/* by now, pid must be in w_stat[0] */
	if(w_njob < JBYTES && w_stat[0].pid)
		w_stat[w_njob++] = w_stat[0];
	w_stat[0].pid = 0;		/* reset temporary location */
}

sav_wstat(p, s)
int p;
unsigned short s;
{
	register int i;

	for(i=0; i<w_njob; i++)
		if(w_stat[i].pid == p)
			w_stat[i].exstat = s;	/* save exit status */
}
#endif /* POSIX2 */
