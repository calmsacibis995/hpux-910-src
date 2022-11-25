/* @(#) $Revision: 66.1 $ */      

/****************************************************************************

	DEBUGGER

****************************************************************************/
#include "defs.h"

MSG		NOFORK;
MSG		NOPCS;
MSG		OPABORT;
FILE		*outfile;
CHAR		*lp;
int		errno;
int		(*sigint)();
int		(*sigqit)();
STRING		signals[];
BKPTR		lbpt;
BKPTR		bkpthead;
REGLIST		reglist[];
CHAR		lastc;
int		fcor;
int		fsym;
STRING		errflg;
int		signum;
long		dot;
long		var[];
STRING		symfil;
int		wtflag;
int		pid;
long		expv;
int		adrflg;

BKPTR
runpcs(goval)
{
	register BKPTR	bkpt;
	int		code, status, rc = 1;
	unsigned long	count,length;
	unsigned short wait;

	if (!pid) error(NOPCS);
	if (adrflg)
	{
		lbpt = NULL;
		putreg(pid, reglist[pc].roffs, dot);
		reglist[pc].rval = dot;
	}
	if (goval <= 0) setbp();
	putreg(pid, reglist[ps].roffs, reglist[ps].rval);
	if (goval == -2) goval = 0;
	else printf("%s: running\n", symfil);
	if (goval > 0)
	{
		if (lbpt) lbpt = NULL;
		for (; goval > 0; --goval)
		{
		    /* check for dragon instruction */

		    dot = reglist[pc].rval;
		    if (Dragon_count(&count,&length,&wait))
		    {
			for (count--; count > 0; --count)
			{
			   ioctl(0, TCSETAW, &subtty);
			   fcntl(0, F_SETFL, sub_fcntl_flags);
			   ptrace(SINGLE, pid, reglist[pc].rval, signum);
			   if ((code = subwait(&status)) != 0177) break;
			   reglist[pc].rval = getreg(pid, reglist[pc].roffs);
			}
		    }

			ioctl(0, TCSETAW, &subtty);
			fcntl(0, F_SETFL, sub_fcntl_flags);
			ptrace(SINGLE, pid, reglist[pc].rval, signum);
			if ((code = subwait(&status)) != 0177) break;
			reglist[pc].rval = getreg(pid, reglist[pc].roffs);
		}
	}
	else while (rc--)
	{
		if (lbpt && lbpt->flag)
		{
			code = execbkpt(lbpt);
			rc++;
		}
		else code = runwait(reglist[pc].rval, &status);
		reglist[pc].rval = getreg(pid, reglist[pc].roffs);
		if ((code == 0177) && (bkpt = scanbkpt(reglist[pc].rval-2))
			&& (bkpt != lbpt))
		{
			/*stopped at bkpt*/
			lbpt = bkpt; reglist[pc].rval = bkpt->loc;
			putreg(pid, reglist[pc].roffs, bkpt->loc);
			if (bkpt->flag == BKPTSET)
			{
				if (--bkpt->count) rc++;
				else bkpt->count = bkpt->initcnt;
			}
			else rc = 0;
			if (*(bkpt->comm) != EOR)
			{	readregs();
				delbp();
				printf("%s",bkpt->comm);
				command(bkpt->comm,':');
				setbp();
			}
		}
		else lbpt = NULL;
	}
	readregs(); if (rc < 0) delbp();
	if (!lbpt) printf("stopped at\t");
	else if (lbpt->flag == BKPTSET) printf("breakpoint\t");
	else if (lbpt->flag == SINGLE)
	{
		printf(":S stopped at\t");
		lbpt->flag = 0; lbpt = 0;
	}
	else
	{
		printf("subroutine call completed"); printc(EOR);
		putreg(pid, reglist[pc].roffs, lbpt->flag);
		putreg(pid, reglist[sp].roffs, lbpt->initcnt);
		reglist[pc].rval = lbpt->flag;
		reglist[sp].rval = lbpt->initcnt;
		dot = lbpt->count; lbpt->flag = 0;
		lbpt = scanbkpt(dot);
		return(lbpt);
	}
	printpc();
	return(lbpt);
}

endpcs()
{
	register BKPTR	bkptr;

	if (pid)
	{
		printf("process %d killed\n", pid);
		ptrace(EXIT, pid, 0, 0); pid = 0;
		for(bkptr = bkpthead; bkptr; bkptr = bkptr->nxtbkpt)
			if (bkptr->flag != BKPTSET) bkptr->flag = 0;
		if (fcor != -1) readregs();
	}
}

setup(psize)
{
	int	status;

	close(fsym); fsym = -1;
	if ((pid = fork()) == 0)
	{
		ptrace(PT_SETTRC, 0, 0, 0);
		signal(SIGINT, sigint);
		signal(SIGQUIT, sigqit);
		doexec(); exit(0);
	}
	else if (pid == -1)
	{
		pid = 0;
		error(NOFORK);
	}
	subwait(&status);
	lbpt = NULL; readregs();
	fsym = open(symfil, wtflag);
	lp[0] = EOR; lp[1] = NULL;
	printf("process %d created\n", pid);
}

execbkpt(bkptr)
register BKPTR	bkptr;
{
	int	status, code;

 	put(bkptr->loc, ISP, bkptr->ins);
	ioctl(0, TCSETAW, &subtty);
	fcntl(0, F_SETFL, sub_fcntl_flags);
	ptrace(SINGLE, pid, bkptr->loc, signum);
	code = subwait(&status);
	put(bkptr->loc, ISP, BKPTI);
	return(code);
}

doexec()
{
	char		*argl[MAXARG];
	char		args[LINSIZ];
	register char	*p, **ap, *fname;

	ap = argl; p = args; *ap++ = symfil;
	do
	{
		rdc();
		if (lastc == EOR) break; *ap = p;
		while ((lastc != EOR) && (lastc != SPACE) && (lastc != TB))
		{
			*p++ = lastc;
			readchar();
		}
		*p++ = NULL; fname = *ap + 1;
		if (**ap == '<')
		{
			if (*fname == 0)
				get_name(fname);
			close(0);
			if (open(fname, 0) < 0)	
			{
				fprintf(stderr,"cannot open input file: %s\n", fname);
				exit(0);
			}
		}
		else if (**ap == '>')
		{
			if (*fname == 0)
				get_name(fname);
			close(1);
			if (creat(fname, 0666) < 0)
			{
				fprintf(stderr,"cannot create output file: %s\n", fname);
				exit(0);
			}
		}
		else ap++;
	}
	while (lastc != EOR);
	
	*ap++ = NULL;
	execv(symfil, argl);
}

BKPTR
scanbkpt(adr)
{
	register BKPTR	bkptr;

	for(bkptr = bkpthead; bkptr; bkptr = bkptr->nxtbkpt)
	if (bkptr->flag && (bkptr->loc == adr)) break;
	return(bkptr);
}

clearbkpts()
{
	register BKPTR	bkptr;

	for(bkptr = bkpthead; bkptr; bkptr = bkptr->nxtbkpt)
		bkptr->flag = 0;
}

delbp()
{
	register BKPTR	bkptr;

	if (pid)
	for(bkptr = bkpthead; bkptr; bkptr = bkptr->nxtbkpt)
		if (bkptr->flag) put(bkptr->loc, ISP, bkptr->ins);
}

setbp()
{ 	int bkfail = 0;
	register BKPTR	bkptr;

	if (pid)
	for(bkptr = bkpthead; bkptr; bkptr = bkptr->nxtbkpt)
	if (bkptr->flag)
	{	bkptr->ins = (get(bkptr->loc, ISP)<<16) & 0xFFFF0000;
		put(bkptr->loc, ISP, BKPTI);
		if (errno)
		{	prints("cannot set breakpoint: ");
			psymoff(bkptr->loc, ISYM, "\n");
			bkfail = 1;
		}
	}
	if (bkfail)
	{	delbp();
		error(OPABORT);
	}
}

readregs()
{
	register REGPTR	regptr;

	for(regptr = reglist; regptr <= &reglist[pc]; regptr++)
		regptr->rval = getreg(pid, regptr->roffs);
}

get_name(p)
register char *p;
{	rdc();
	while ((lastc != EOR) && (lastc != SPACE) &&
		(lastc != TB))
	{
		*p++ = lastc;
		readchar();
	}
	*p++ = 0;
}
