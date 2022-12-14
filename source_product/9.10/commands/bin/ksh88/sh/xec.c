/* HPUX_ID: @(#) $Revision: 72.3 $ */
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
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten by David Korn
 * AT&T Bell Laboratories
 *
 */

#include	<errno.h>
#include	"defs.h"
#include	"jobs.h"
#include	"sym.h"
#include	"test.h"
#include	"builtins.h"
#include	<sys/time.h>

#ifdef _sys_timeb_
#   define TIC_SEC		60
#   include	<sys/timeb.h>
#endif	/* _sys_timeb_ */

#ifdef HP_DEBUG
#include	"debug.h"
#endif /* HP_DEBUG */

/* These routines are referenced by this module */
#ifdef DEVFD
    extern void	close_pipes();
#endif	/* DEVFD */
#ifdef VFORK
    extern int	vfork_check();
    extern int	vfork_save();
    extern void	vfork_restore();
#endif	/* VFORK */

static int trim_eq();
static char *word_trim();
static char locbuf[TMPSIZ]; 	/* store last argument here if it fits */
static char pipeflag;

/* ========	command execution	========*/

sh_exec(argt, flags)
union anynode 	*argt;
int flags;
{
	/* `stakbot' is preserved by this routine */
	register union anynode *t;
	register int 	type = flags;
	STKPTR 	sav=stak_word();
	int errorflg = (type&ERRFLG);
	int execflg = (type&1);
	int lastpipe = (type&LASTPIPE);
	unsigned save_states = (st.states&(ERRFLG|MONITOR|LASTPIPE));
#ifdef VFORK
	int v_fork;
#endif	/* VFORK */
	if(sh.trapnote&SIGSET)
		sh_exit(SIGFAIL);
	flags &= ~(LASTPIPE|PROMPT);
	st.states &= ~(ERRFLG|MONITOR);
	st.states |= ((save_states&flags)|lastpipe);
	if((t=argt) && st.execbrk==0 && !is_option(NOEXEC))
	{
		char	**com;
		int	argn;
		char *com0 = NIL;
		pid_t forkpid;
		jmp_buf retbuf;
		jmp_buf *savreturn;
		int save;
		int topfd;
		struct fileblk *savio;
		if(lastpipe)
		{
			/* save state for last element of a pipe */
			topfd = sh.topfd;
			forkpid = job.parent;
			if(io_ftable[0])
				save = io_ftable[0]->flag;
			savreturn = sh.freturn;
			sh.freturn = (jmp_buf*)retbuf;
			save_states |= (st.states&PROMPT);
			st.states &= ~PROMPT;
			savio = sh.savio;
			sh.savio = st.standin;
			if(setjmp(retbuf))
				goto endpipe;
			if(!execflg)
				io_save(0,topfd);
			io_renumber(sh.inpipe[0],0);
		}
		type = t->tre.tretyp;
		sh.oldexit=sh.exitval;
		sh.exitval=0;
		sh.lastpath = 0;
		switch(type&COMMSK)
		{
			case TCOM:
			{
				register struct ionod	*io;
				register struct argnod	*argp;
#ifdef POSIX2
				/* for the command built-in */
				int command = 0;
#endif /* POSIX2 */
				struct namnod* np;
				type >>= (COMBITS+1);
				st.cmdline = t->com.comline;
				com = arg_build(&argn,&(t->com));
				if(t->tre.tretyp&COMSCAN)
				{
					argp = t->com.comarg;
					if(argp && (argp->argflag&A_RAW)==0)
					{
						type = sh_lookup(com[0],tab_builtins);
#ifdef HP_BUILTIN
						/*
						 *  if we didn't find it in the normal
						 *  builtins, and we have done the set
						 *  -o hp_builtin then check hp_builtins
						 */
						if(!type && hp_builtin)
						   type = sh_lookup(com[0],tab_hp_builtins);
#endif /* HP_BUILTIN */						
					}
					if(sh.trapnote&SIGSET)
						sh_exit(SIGFAIL);
				}
#ifdef POSIX2
				/*
				 *  Do the check for the command built-in
				 */
				while(type==SYSCOMMAND)
				{
					command++;
					com++;
					argn--;
					type = sh_lookup(com[0],tab_builtins);
				}
#endif /* POSIX2 */
				com0 = com[0];
				io = t->tre.treio;
				if(argp = t->com.comset)
				{
					if(argn==0 || (!st.subflag && type>0 && type<=SYSSPECIAL))
					{
						env_setlist(argp, 0);
						argp = NULL;
					}
				}
				if((io||argn))
				{
					if(argn==0)
					{
						/* fake a built-in */
						argn=1;
						type = SYSMAX;
					}
					/* set +x doesn't echo */
					else if((type!=SYSSET) && is_option(EXECPR))
#ifdef POSIX2
						/* for command built-in */
						sh_trace(com-command,1);
#else
						sh_trace(com,1);
#endif /* POSIX2 */
					if(io)
						p_flush();
#ifdef apollo
					if(is_option(INPROC) &&  type==0 
						&& (st.states&FORKED)==0 &&
						!nam_search(com0,sh.fun_tree,0))
					{
						type = SYSINPROCESS;
						com--;
						argn++;
					}
#endif	/* apollo */
#ifdef POSIX2
					/*
					 * For POSIX, the command lookup order
					 * is:
					 *    alias function built-in $PATH
					 *
					 * but for the command built-in, we skip 
					 * the lookup of functions.
					 */
					if(!command)
					    goto dofunction;
				dobuiltin:
#endif /* POSIX2 */
					/* check for builtins */
					if(type)
					{
						/* failures on built-ins not fatal */
						jmp_buf retbuf;
						jmp_buf *savreturn = sh.freturn;
						struct fileblk *saveio=sh.savio;
						int indx = sh.topfd;
						int scope=0;
						if(st.subflag || type>SYSSPECIAL)
							sh.freturn = (jmp_buf*)retbuf;
						sh.savio = st.standin;
						if(setjmp(retbuf) == 0)
						{
							int flag = execflg;
							if(type<SYSNULL)
							{
								if(type==SYSLOGIN)
									flag=1;
								else if(type==SYSEXEC)
									flag=1+(com[1]==0);
							}
							else if(type>=SYSSPECIAL)
								st.states |= BUILTIN;
							if(io)
								indx = io_redirect(io,flag);
							if(argp)
							{
								nam_scope(argp);
								scope++;
							}
							sh_builtin(type,argn,com,t);
						}
						sh.savio = saveio;
						st.states &= ~BUILTIN;
						sh.freturn = savreturn;
						if(scope)
							nam_unscope();
						if(io)
							io_restore(indx);
						goto setexit;
					}
#ifdef POSIX2
					/*
					 *  command lookup order is different for
					 *  POSIX.
					 */
					goto dosearch;
#endif /* POSIX2 */
					/* check for functions */
				dofunction:
					if(np=nam_search(com0,sh.fun_tree,0))
					{
						int indx;
						if(np->value.namval.ip==0)
						{
							if(!nam_istype(np,N_FUNCTION))
#ifdef POSIX2
								goto dobuiltin;
#else
								goto dosearch;
#endif /* POSIX2 */
							path_search(com0,0);
							if(np->value.namval.ip==0)
								sh_fail(com0,e_found);
						}
						indx = io_redirect(io,execflg);
						sig_ignore(SIGWINCH);
						sh_funct((union anynode*)(funtree(np))
							,com
							,flags|(int)(nam_istype(np,T_FLAG)?EXECPR:0)
							,t->com.comset);
						sig_ontrap(SIGWINCH);
						io_restore(indx);
						goto setexit;
					}
#ifdef POSIX2
					goto dobuiltin;
				dosearch:
#endif /* POSIX2 */
					if(strchr(com0,'/')==0)
					{
#ifndef POSIX2
					dosearch:
#endif /* POSIX2 */
						if(path_search(com0,1))
							goto dofunction;
					}
				}
				else if(io==0)
				{
				setexit:
					sh.exitval &= EXITMASK;
					exitset();
					/* DSDe409061
					 * needs to check DEBUG trap for
					 * builtin commands also.
					 */
					if(sh.trapnote ||
					   st.trapflg[DEBUGTRAP] ||
					   (sh.exitval && (st.states&ERRFLG)))
						sh_chktrap();
					break;
				}
				type = TCOM;
			}
			case TFORK:
			{
				register pid_t parent;
				int no_fork;
				int pipes[2];
				io_sync();
				no_fork = (execflg && !(type&(FAMP|FPOU)) &&
					*st.trapcom==0);
				if(no_fork)
					job.parent=parent=0;
				else
				/* FORKLIM is the max period between forks -
					power of 2 usually.  Currently shell tries after
					2,4,8,16, and 32 seconds and then quits */
				{
					register unsigned forkcnt=1;
					if(type&FTMP)
					{
						io_linkdoc(st.iotemp);
						st.linked = 1;
					}
					if(type&FCOOP)
					/* set up pipe for cooperating process */
					{
						if(sh.cpid)
							sh_fail(e_pexists,NIL);
					/* FSDlj09406: more than one read -p
					 * used to cause core dumps.
					 * Added check for sh.cpipe[1] below
					 */
						if(sh.cpipe[0]<=0 || sh.cpipe[1]<=0)
						{
							/* first co-process */
							io_popen(pipes);
							sh.cpipe[0] = io_renumber(pipes[0],CINPIPE);
							sh.cpipe[1] = io_renumber(pipes[1],CINPIPE2);
						}
						sh.outpipe = sh.cpipe;
						io_popen(sh.inpipe=pipes);
						sh.inpipe[1] = io_renumber(sh.inpipe[1],COTPIPE);
					}
					/* async commands allow job control */
					if((type&FAMP) && !(type&(FCOMSUB|FPOU))
						&&!(st.states&PROFILE))
						st.states |= is_option(MONITOR);
					nam_strval(RANDNOD);
#ifdef VFORK
					if(v_fork=vfork_check(t))
						vfork_save();
					while((job.parent=parent=(v_fork?vfork():fork())) == -1)
#else
					while((job.parent=parent=fork()) == -1)
#endif	/* VFORK */
					{
						if((forkcnt *= 2) > FORKLIM)
						{
							register const char *msg;
							switch(errno)
							{
							case ENOMEM:
								msg = e_swap;
								break;
							default:
							case EAGAIN:
								msg = e_fork;
								break;
							}
							sh_fail(msg,NIL);
						}
						if(sh.trapnote&SIGSET)
							sh_exit(SIGFAIL);
						alarm(forkcnt);
						pause(); 
						alarm(0);
					}
				}
#ifdef SIGTSTP
				if(!job.jobcontrol)
#endif /* SIGTSTP */
				{
					/* disable foreground job control */
					if(!(type&FAMP))
						job_nonstop();
#   ifdef DEVFD
					else if(!(type&FINT))
						job_nonstop();
#   endif /* DEVFD */
				}
				if(flags&MONITOR)
					job.pipeflag = pipeflag;
				else
					job.pipeflag = 0;
				if(parent)
				/* This is the parent branch of fork
				 * It may or may not wait for the child
				 */
				{
					register int pno;
#ifdef VFORK
					if(v_fork)
						vfork_restore();
#endif	/* VFORK */
					forkpid = parent;
#ifdef JOBS
					/*
					 *  The first process in a pipeline sets
					 *  the process group for the pipe.  The
					 *  rest of the processes (job.pipeflag
					 *  == 1) are put in the process group of
					 *  the first process.
					 *
					 *  We don't want command substitution 
					 *  processes to mess up our idea of the
					 *  current job's process group, so we don't
					 *  allow them to change the pgid (they are
					 *  run with MONITOR off anyway).  There are
					 *  some changes in job_wait() to account for
					 *  this.
					 */
					if(!job.pipeflag && ((hp_flags1&HPCOMSUB) == 0))
						job.curpgid = parent;

					/*
					 *  To avoid race conditions with the
					 *  child, we have both the parent and
					 *  the child set up the process group
					 */
					if (st.states&MONITOR)
					{
						if(job.pipeflag == 0)
							setpgid(parent, parent);
						else
							setpgid(parent, job.curpgid);
					}
#endif /* JOBS */
					if(lastpipe && (st.states&MONITOR))
					{
						io_restore(topfd);
						forkpid = 0;
						execflg++;
					}
					if(type&FPCL)
						close(sh.inpipe[0]);
					if(type&FCOOP)
						sh.cpid = parent;
					else if(type&FCOMSUB)
						sh.subpid = parent;
					pno = job_post(parent);
					if(lastpipe)
						pipeflag = 0;
					if((type&(FAMP|FPOU))==0)
					{
#ifdef DEVFD
						close_pipes();
#endif	/* DEVFD */
						job_wait(parent);
						/* invalidate tty state */
						tty_set(-1);
					}
					if(type&FAMP)
					{
						sh.bckpid = parent;
						job.curpgid = 0;
						if(st.states&(PROFILE|PROMPT))
						{
							/* print job number */
							p_setout(ERRIO);
#ifdef JOBS
							p_sub(pno,'\t');
#endif /* JOBS */
							p_num(parent,NL);
						}
					}
					/*
					 *  Since sh_chktrap may do a job_wait()
					 *  and reap some zombies, we don't want
					 *  to call it while we are setting up a
					 *  pipe.  The danger is that it will 
					 *  reap the last process in the process
					 *  group of the pipe and subsequent
					 *  setpgid()s will fail, leaving the 
					 *  rest of the processes in the wrong
					 *  process group.
					 */
					 if(!((t->tre.tretyp)&TFIL))
						sh_chktrap();
					break;
				}
				else
				/*
				 * this is the FORKED branch (child) of execute
				 */
				{
					register pid_t pid = getpid();
#ifdef HP_DEBUG

					if(hp_debug & FORK_DEBUG)
					{
						p_setout(ERRIO);
						p_num(getpid(),NL);
						p_flush();
						while(1);
				       }
#endif /* HP_DEBUG */
					if(st.standout != 1)
						st.standout = io_renumber(st.standout,1);
					st.states |= (FORKED|NOLOG);
					if(no_fork==0)
					{
						/*
						 *  We really did fork.
						 *
						 *  We set sh.login_sh to 0 
						 *  so that built-ins forked
						 *  from a login shell do not
						 *  send a SIGHUP to all the
						 *  jobs in the inherited jobs
						 *  structures when the forked
						 *  built-in goes through sh_done()
						 */

						sh.login_sh = 0;
						hp_flags1 &= ~HPCOMSUB;
						st.states &= ~(RM_TMP|IS_TMP);
						io_settemp(pid);
						if(st.linked == 1)
						{
							io_swapdoc(st.iotemp);
							st.linked = 2;
						}
						else
							st.iotemp=0;
						job.waitsafe = 0;
					}
					else if(st.states&RM_TMP)
						rm_files(io_tmpname);
#ifdef ACCT
					suspacct();
#endif	/* ACCT */
					/* child should not unlink the tmpfile */
					/* Turn off INTR and QUIT if `FINT'  */
					/* Reset remaining signals to parent */
					/* except for those `lost' by trap   */
#ifdef VFORK
					sig_reset(1);
#else
					sig_reset(0);
#endif /* VFORK */
#ifdef JOBS
					if(st.states&MONITOR)
					{
						int pgrp;
						if(job.pipeflag==0)
							pgrp = pid;
						else
							pgrp = job.curpgid;
						setpgid(0,pgrp);
						if(!job.jobcontrol&&(type&FINT))
							goto closein;
#   ifdef SIGTSTP
						if(!(type&FAMP)) {
						 if (tcsetpgrp(jobtty,pgrp)==-1
						   && errno==ENOTTY) {
						  jobtty = ++jobtty%3;
						  if (tcsetpgrp(jobtty,pgrp)==-1
						   && errno==ENOTTY) {
						    jobtty = ++jobtty%3;
						    tcsetpgrp(jobtty,pgrp);
						  }
						 }
						}
						/*
						 *  This fixes FSDlj06463.  When the
						 *  editor spawned by fc was stopped,
						 *  the commands were executed at that
						 *  point, rather than when it was resumed
						 *  and really exited, so now we don't allow
						 *  it to be stopped.  This is a stop-gap
						 *  measure until there is time to do the
						 *  real solution.
						 */
						if (st.states&NONSTOP  ||  hp_flags1&DOING_FC)
							signal(SIGTSTP,SIG_IGN);
						else
						{
							signal(SIGTSTP,SIG_DFL);
							signal(SIGTTIN,SIG_DFL);
							signal(SIGTTOU,SIG_DFL);
						}
#   endif /* SIGTSTP */
					}
					else if(type&FINT)
#else
					if(type&FINT)
#endif /* JOBS */
					{
						/* default std input for & */
						signal(SIGINT,SIG_IGN);
						signal(SIGQUIT,SIG_IGN);
					closein:
						if(!st.ioset)
						{
							close(0);
							io_fopen(e_devnull);
						}
					}
					/* pipe in or out */
					if((type&FAMP) && is_option(BGNICE))
						nice(4);
					if(type&FPIN)
					{
						io_renumber(sh.inpipe[0],0);
						if((type&FPOU)==0||(type&FCOOP))
							close(sh.inpipe[1]);
					}
					if(type&FPOU)
					{
						io_renumber(sh.outpipe[1],1);
						io_pclose(sh.outpipe);
					}
					st.states &= ~MONITOR;
#ifdef JOBS
					job.jobcontrol = 0;
#endif /* JOBS */
					pipeflag = 0;
					if(type!=TCOM)
						st.cmdline = t->fork.forkline;
					io_redirect(t->tre.treio,1);
					if(type!=TCOM)
					{
						/* don't clear job table for out
						   pipes so that jobs can be
						   piped
						 */
						if(no_fork==0 && (type&FPOU)==0)
							job_clear();
						sh_exec(t->fork.forktre,flags|1);
					}
					else if(com0!=ENDARGS)
					{
						off_option(ERRFLG);
						io_rmtemp((struct ionod*)0);
						path_exec(com,t->com.comset);
					}
					sh_done(0);
				}
			}

			case TSETIO:
			{
			/*
			 * don't create a new process, just
			 * save and restore io-streams
			 */
				int indx;
				st.cmdline = t->fork.forkline;
				indx = io_redirect(t->fork.forkio,execflg);
				sh_exec(t->fork.forktre,flags);
				io_restore(indx);
				break;
			}

			case TPAR:
			{
			/*
			 *  Parenthesis subshells.
			 *
			 *  Set up a new jmp_buf so that
			 *  a "return" does not cause the shell to
			 *  return to exfile() and the main loop. 
			 */
				jmp_buf *save_return;
				jmp_buf return_buf;
				
				save_return = sh.freturn;
				sh.freturn = (jmp_buf *)return_buf;
				if(setjmp(return_buf)==0)
					sh_exec(t->par.partre,flags);
				sh.freturn = save_return;
				sh_done(0);
			}
			
			case TFIL:
			{
			/*
			 * This code sets up a pipe.
			 * All elements of the pipe are started by the parent.
			 * The last element executes in current environment
			 */
				register union anynode *tf;
				int pvo[2];	/* old pipe for multi-pipeline */
				int pvn[2];	/* set up pipe */
				type = 1;
				pipeflag = 0;
				sh.inpipe = pvo;
				sh.outpipe = pvn;
				do
				{
					/* create the pipe */
					io_popen(pvn);
					tf = t->lst.lstlef;
					/* type==0 on multi-stage pipe */
					if(type==0)
						tf->fork.forktyp |= FPCL|FPIN;
					/* execute out part of pipe no wait */
					type = sh_exec(tf, MONITOR|errorflg);
					tf = t->lst.lstrit;
					t = tf->fork.forktre;
					/* save the pipe stream-ids */
					pvo[0] = pvn[0];
					/* close out-part of pipe */
					close(pvn[1]);
					/* pipeline all in one process group */
					job.topfd = sh.topfd;
					pipeflag++;
				}
				/* repeat until end of pipeline */
				while(!type && !tf->fork.forkio && t->tre.tretyp==TFIL);
				sh.inpipe = pvn;
				sh.outpipe = 0;
				if(type == 0)
					/*
					 * execute last element of pipeline
					 * in the current process
					 */
					sh_exec(tf->fork.forktre,flags|(LASTPIPE));
				else
					/* execution failure, close pipe */
					io_pclose(pvn);
				break;
			}
	
			case TLST:
			{
				/*  a list of commands are executed here */
				do
				{
					sh_exec(t->lst.lstlef,MONITOR|errorflg);
					t = t->lst.lstrit;
				}
				while(t->tre.tretyp == TLST);
				sh_exec(t,flags);
				break;
			}
	
			case TAND:
				if(sh_exec(t->lst.lstlef,0)==0)
				{
					st.states |= NONSTOP;
					sh_exec(t->lst.lstrit,flags);
					st.states &= ~NONSTOP;
				}
				break;
	
			case TORF:
				if(sh_exec(t->lst.lstlef,0)!=0)
				{
					st.states |= NONSTOP;
					sh_exec(t->lst.lstrit,flags);
					st.states &= ~NONSTOP;
				}
				break;
	
			case TFOR:
			case TSELECT:
			{
				register char **args;
				register int nargs;
				struct namnod *n;
				char **arglist;
				struct dolnod	*argsav=NULL;
				struct comnod	*tp;
				char *nullptr = NULL;
				if((tp=t->for_.forlst)==NULL)
				{
					args=st.dolv+1;
					nargs = st.dolc;
					argsav=arg_use();
				}
				else
				{
					args=arg_build(&argn,tp);
					nargs = argn;
				}
				if(type==TSELECT)
				{
					p_setout(ERRIO);
					p_list(nargs,arglist=args,0);
				}
				n = env_namset(t->for_.fornam, sh.var_tree,P_FLAG|V_FLAG);
				st.loopcnt++;
				while(*args !=ENDARGS && st.execbrk == 0)
				{
					if(t->tre.tretyp==TSELECT)
					{
						char *val,*cp;
						/* reuse register */
#define c	type
						sh_prompt(1);
						env_readline(&nullptr,0,R_FLAG);
						if(fiseof(io_ftable[0]))
						{
							sh.exitval = 1;
							clearerr(io_ftable[0]);
							break;
						}
						if((val=nam_fstrval(REPLYNOD))==NULL)
							continue;
						else
						{
							if(*(cp=val) == 0)
							{
								p_list(nargs,args,0);
								goto checkbrk;
							}
							while(c = *cp++)
							if(c < '0' && c > '9')
								break;
							if(c!=0)
								c = nargs;
							else
								c = atoi(val)-1;
							if(c<0 || c >= nargs)
								c = nargs;
							args += c;
						}
					}
#undef c
					nam_putval(n, *args);
					if(t->tre.tretyp != TSELECT)
						args++;
					else
						args = arglist;
					st.states |= NONSTOP;
					sh_exec(t->for_.fortre,MONITOR|errorflg);
					st.states &= ~NONSTOP;
				checkbrk:
					if(st.breakcnt<0)
						st.execbrk = (++st.breakcnt !=0);
					}
				if(st.breakcnt>0)
					st.execbrk = (--st.breakcnt !=0);
				st.loopcnt--;
				arg_free(argsav,0);
				break;
			}
	
			case TWH:
			{
				register int 	i=0;
				st.loopcnt++;
				st.states |= NONSTOP;
				while(st.execbrk==0 && (sh_exec(t->wh.whtre,0)==0)==(type==TWH))
				{
					i = sh_exec(t->wh.dotre,MONITOR|errorflg);
					if(st.breakcnt<0)
						st.execbrk = (++st.breakcnt !=0);
				}
				st.states &= ~NONSTOP;
				if(st.breakcnt>0)
					st.execbrk = (--st.breakcnt !=0);
				st.loopcnt--;
				sh.exitval= i;
				break;
			}
	
			case TIF:
				if(sh_exec(t->if_.iftre,0)==0)
					sh_exec(t->if_.thtre,flags);
				else if(t->if_.eltre)
					sh_exec(t->if_.eltre, flags);
				else
					sh.exitval=0; /* force zero exit for if-then-fi */
				break;
	
			case TSW:
			{
				char *r = word_trim(t->sw.swarg);
				t= (union anynode*)(t->sw.swlst);
				while(t)
				{
					register struct argnod	*rex=(struct argnod*)t->reg.regptr;
					while(rex)
					{
						register char *s;
						if(rex->argflag&A_MAC)
							s = mac_expand(rex->argval);
						else
							s = rex->argval;
						type = (rex->argflag&A_RAW);
						if((type && eq(r,s)) ||
							(!type && (strmatch(r,s)
							|| trim_eq(r,s))))
						{
							sh_exec(t->reg.regcom,flags);
							t=0;
							break;
						}
						else
							rex=rex->argnxt.ap;
					}
					if(t)
						t=(union anynode*)t->reg.regnxt;
				}
				break;
			}

			case TTIME:
#ifdef POSIX2
			/*
			 *   This implements the POSIX
			 *   negation reserved word "!".
			 *   see also sh/cmd.c
			 */
				if(type!=TTIME)
				{
					sh_exec(t->par.partre,0);
					sh.exitval = !sh.exitval;
					break;
				}
#endif /* POSIX2 */
			{
				/* time the command */
				struct tms before,after;
				clock_t at,bt;
#ifdef _sys_timeb_
				struct timeb tb,ta;
				ftime(&tb);
#endif	/* _sys_timeb_ */
				bt = times(&before);
				sh_exec(t->par.partre,MONITOR);
				at = times(&after);
#ifdef _sys_timeb_
				ftime(&ta);
				at = CLK_TCK*(ta.time-tb.time);
				at +=  ((CLK_TCK*((long)(ta.millitm-tb.millitm)))/1000);
#else
				at -= bt;
#endif	/* _sys_timeb_ */
				p_setout(ERRIO);
				p_str(e_real,'\t');
				p_time(at,NL);
				p_str(e_user,'\t');
				at = after.tms_utime - before.tms_utime;
				at += after.tms_cutime - before.tms_cutime;
				p_time(at,NL);
				p_str(e_sys,'\t');
				at = after.tms_stime - before.tms_stime;
				at += after.tms_cstime - before.tms_cstime;
				p_time(at,NL);
				break;
			}
			case TPROC:
			{
				register struct namnod *np;
				register char *fname = ((struct procnod*)t)->procnam;
				if(!isalpha(*fname))
					sh_fail(fname,e_ident);
				np = env_namset(fname,sh.fun_tree,P_FLAG|V_FLAG);
				if(np->value.namval.rp)
				{
					register struct blk *bp;
					bp = (struct blk*)(np->value.namenv);
					while(bp)
					{
						free((char*)bp);
						bp = bp->word;
					}
				}
				else
					np->value.namval.rp = new_of(struct Ufunction,0);
				if(t->proc.procblk)
				{
					np->value.namenv = (char*)(t->proc.procblk);
					funtree(np) = (int*)(t->proc.proctre);
					np->value.namval.rp->hoffset = t->proc.procloc;
				}
				else
				{
					free((char*)np->value.namval.rp);
					np->value.namval.rp = 0;
				}
				nam_ontype(np,N_FUNCTION);
				break;
			}

#ifdef NEWTEST
			/* new test compound command */
			case TTST:
			{
				register int n;
				int nargs;
				char *left;
				/* we need the following for execution trace */
				static char *arg[6]=  {"[[ !"};
				static char unop[3]= "-?";
				;
				if (type&TPAREN)
				{
					sh_exec(t->lst.lstlef,0);
					n = !sh.exitval;
				}
				else
				{
					n = type>>TSHIFT;
					left = word_trim(&(t->lst.lstlef->arg));
					if(type&TUNARY)
					{
						nargs = 3;
						unop[1] = n;
						arg[1] = unop;
						arg[2] = left;
						n = unop_test(n,left);
					}
					else if(type&TBINARY)
					{
						nargs = 4;
						arg[1] = left;
						arg[2] = (char*)(test_optable+(n&017)-1)->sysnam;
						arg[3] = word_trim(&(t->lst.lstrit->arg));
						n = test_binop(n,left,
							word_trim(&(t->lst.lstrit->arg)));
					}
					arg[nargs] = "]]";
					arg[nargs+1] = 0;
					if(type&TNEGATE)
						arg[0][2] = ' ';
					else
						arg[0][2] = 0;
					sh_trace(arg,1);
				}
				sh.exitval = (!n^((type&TNEGATE)!=0));
				break;
			}
#endif /* NEWTEST */
		}
#ifndef POSIX2
		/* "_" is not a special variable for POSIX shell
		 * DSDe406983 .. ifdef'ed this out.
		 */
		/* set $_ */
		if(com0)
		{
			if(sh.lastarg!= locbuf)
				free(sh.lastarg);
			if(strlen(com[argn-1]) < TMPSIZ)
				sh.lastarg = strcpy(locbuf,com[argn-1]);
			else
				sh.lastarg = sh_heap(com[argn-1]);
		}
#endif
		if(lastpipe)
		{
		endpipe:
			if(!execflg)
			{
				io_restore(topfd);
				type = sh.exitval;
				job_wait(forkpid);
				sh.exitval = type;
			}
			if(io_ftable[0])
				io_ftable[0]->flag = save;
			sh.savio = savio;
			st.states &= ~LASTPIPE;
			sh.freturn = savreturn;
			pipeflag = 0;
		}
		exitset();
	}
	stak_reset(sav);
	st.states |= save_states;
	st.linked = 0;
	if(sh.trapnote&SIGSET)
		sh_exit(SIGFAIL|sh.exitval);
	return(sh.exitval);
}

/*
 * test for equality with second argument trimmed
 * returns 1 if r == trim(s) otherwise 0
 */

static trim_eq(r,s)
register char *r,*s;
{
	register char c;
	while(c = *s++)
	{
		if(c==ESCAPE)
			c = *s++;
		if(c && c != *r++)
			return(0);
	}
	return(*r==0);
}

/*
 * print out the command line if set -x is on
 */

int sh_trace(com,nl)
char **com;
{
	if(is_option(EXECPR))
	{
		p_setout(ERRIO);
		p_str(mac_try(nam_fstrval(PS4NOD)),0);
		if(com)
			echo_list(1,com);
		if(nl)
			newline();
		return(1);
	}
	return(0);
}


static char *word_trim(arg)
register struct argnod *arg;
{
	register char *r = arg->argval;
	if(!(arg->argflag&A_RAW))
		r = mac_trim(r,0);
	return(r);
}

