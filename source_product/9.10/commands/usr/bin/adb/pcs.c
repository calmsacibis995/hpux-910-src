/* @(#) $Revision: 66.1 $ */     

/****************************************************************************

	DEBUGGER - process control loop

****************************************************************************/
#include "defs.h"

MSG		NOBKPT;
MSG		SZBKPT;
MSG		EXBKPT;
MSG		NOPCS;
MSG		BADMOD;
MSG		BADSYN;
BKPTR		bkptr;
BKPTR		bkpthead;
BKPTR		runpcs();
CHAR		*lp;
CHAR		*printptr;
CHAR		lastc;
REGLIST		reglist[];
long		entrypt;
long		expv;
long		dot;
long		dotinc;
int		pid;
int		adrflg;
long		cntval;
int		signum;
extern int	lastcom;

subpcs(modif)
{
	int		xarg[10];
	register int	check, cnt = cntval;
	STRING		comptr;
	register int	*stkptr;
	extern	char	*malloc();

	switch(modif)
	{
	    /* delete breakpoint */
	    case 'd': 
	    case 'D':
		nextchar();
		if (lastc == '*')
		{	clearbkpts();
			return;
		}
		if (bkptr = scanbkpt(dot))
		{
			bkptr->flag = 0;
			return;
		}
		else error(NOBKPT);

	    /* set breakpoint */
	    case 'b': 
	    case 'B':
		if (bkptr = scanbkpt(dot)) bkptr->flag = 0;
		for (bkptr = bkpthead; bkptr; bkptr = bkptr->nxtbkpt)
			if (bkptr->flag == 0) break;
		if (bkptr == 0)
		{
			if ((bkptr = (BKPTR)malloc(sizeof *bkptr)) == NULL)
				error(SZBKPT);
			else
			{
				bkptr->nxtbkpt = bkpthead;
				bkpthead = bkptr;
			}	
		}
		bkptr->loc = dot;
		bkptr->initcnt = bkptr->count = cntval;
		bkptr->flag = BKPTSET;

		check = MAXCOM - 1; comptr = bkptr->comm; rdc(); lp--;
		do *comptr++ = readchar(); while (check-- && (lastc != EOR));
		*comptr = NULL; lp--;
		if (check) return; else error(EXBKPT);

	    /* exit */
	    case 'k' :
	    case 'K':
		if (pid)
		{
			endpcs();
			return;
		}
		else error(NOPCS);

	    /* set program */
	    case 'e': 
	    case 'E':
		endpcs(); setup(cnt);
		break;

	    /* run program */
	    case 'r': 
	    case 'R':
		endpcs(); setup(cnt);
		bkptr = runpcs(0);
		break;

	    /* single step */
	    case 's':
		lastcom = ':';
		signum = (expr(0) ? expv : signum);
		bkptr = runpcs(cnt);
		break;

	    /* single step across subroutine */
	    case 'S':
		lastcom = ':';
		signum = (expr(0) ? expv : signum);
		dot = reglist[pc].rval;
		printins(0, ISP, get(dot, ISP)); printptr -= charpos();
		dot += dotinc; subpcs('b'); dot -= dotinc;
		bkptr->flag = SINGLE;
		bkptr = runpcs(-1);
		break;

	    /* execute subroutine */
	    case 'x': 
	    case 'X':
		if (!pid) error(NOPCS); cnt = 0; check = dot;
		while (expr(0) && (cnt < 10)) xarg[cnt++] = expv;
		if ((!cnt) || (cnt >= 10)) error(BADSYN);
		if (lastc == ';') rdc();

		dot = entrypt;
		stkptr = (int *)getreg(pid, reglist[sp].roffs);
		subpcs('b');
			bkptr->flag = getreg(pid, reglist[pc].roffs);
			bkptr->count = check;
			bkptr->initcnt = (int)stkptr;

		while (--cnt) ptrace(PT_WDUSER, pid, --stkptr, xarg[cnt]);
		ptrace(PT_WDUSER, pid, --stkptr, dot);
		putreg(pid, reglist[sp].roffs, stkptr);
		dot = xarg[0]; adrflg = TRUE;
		printf("\nrunning subroutine:\n");
		bkptr = runpcs(-2);
		break;

	    /* continue */
	    case 'c': 
	    case 'C': 
		lastcom = ':';
		signum = (expr(0) ? expv : signum);
		while (cnt--) bkptr = runpcs(0);
		break;
	    
	    case 0:
		printf("\nincomplete command. try again.");
		break;

	    default: error(BADMOD);
	}
}

