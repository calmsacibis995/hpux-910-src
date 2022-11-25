/* @(#) $Revision: 66.3 $ */     

/****************************************************************************

	DEBUGGER - sub process creation and setup

****************************************************************************/
#include "defs.h"
#include <setjmp.h>

#define	CALL2	0x4E90		/* jsr a0@	*/
#define CALL6	0x4EB9		/* jsr <adr>	*/
#define ADDQL	0x500F		/* addql #x,sp	*/
#define ADDL	0xDEFC		/* addl #X,sp	*/
#define LEA	0x4fef		/* lea X(sp),sp */
#define ISYM	2
#define LBSR	0x61ff
#define lBSR	0x6100

MSG	BADWAIT;
MSG	NOPCS;
STRING	signals[];
STRING	corfil;
jmp_buf	env;
int	pid = 0;
int	fcor;
long	mainsval;
int	signum;
int	(*sigint)();
int	(*sigqit)();
PROC_REGS *cregs;

runwait(addr, statusp)
int	addr;
int	*statusp;
{
	ioctl(0, TCSETAW, &subtty);
	fcntl(0, F_SETFL, sub_fcntl_flags);
	ptrace(PT_CONTIN, pid, addr, signum);
	return(subwait(statusp));
}

subwait(statusp)
int	*statusp;
{
	int	code;

	signal(SIGINT, SIG_IGN);
	while (((code = wait(statusp)) != pid) && (code != -1));
	signal(SIGINT, sigint);
	ioctl(0, TCGETA, &subtty);
	sub_fcntl_flags = fcntl(0, F_GETFL);
	ioctl(0, TCSETAW, &adbtty);
	fcntl(0, F_SETFL, adb_fcntl_flags);
	if (code == -1)
	{
		pid = 0;
		error(BADWAIT);
	}
	else code = *statusp & 0177;
	/* *statusp will have low byte=0177 iff child process is stopped MFM */
	if (code != 0177)
	{
		pid = 0;
		if (signum = code) prints(signals[signum]);
		/* (*statusp&0200)<>0 iff a core was made MFM */
		if (*statusp & 0200)
		{
			prints(" - core dumped\n");
			close(fcor); corfil = "core"; setcor();
		}
		printf("process terminated\n");
		if (fcor != -1) readregs();
		longjmp(env);
	}
	else
	{
		signum = (*statusp >> 8) & 0377;
		if ((signum != SIGTRAP) && (signum != SIGIOT))
			prints(signals[signum]);
		else signum = 0;
		flushbuf();
	}
	return(code);
}

getreg(pid, offset)
{
	register int	data;
	int	off;

	/* KERNEL_TUNE_DLB */
	if (offset == PS) off = 16*4;
	else if (offset == PC) off = 16*4 +2;
	else off = offset * 4;
	/* KERNEL_TUNE_DLB */

	if (pid) {
		data = ptrace(PT_RUAREA, pid, &(((struct user *)0)->u_ar0), 0);
		data = ptrace(PT_RUAREA, pid, (data - uarea_address) + off, 0);
	}
	else data = *(int *)(((char *)&(cregs->hw_regs)) + off);

	/* KERNEL_TUNE_DLB */
	if (offset == PS) data = (data >> 16) & 0xffff;
	/* KERNEL_TUNE_DLB */

	return(data);
}

putreg(pid, offset, data)
{
	register int t;
	int off;

	/* KERNEL_TUNE_DLB */
	if (offset == PS) off = 16*4;
	else if (offset == PC) off = 16*4 +2;
	else off = offset * 4;
	/* KERNEL_TUNE_DLB */

	if (pid) {
		t = ptrace(PT_RUAREA, pid, &(((struct user *)0)->u_ar0), 0);
		data = ptrace(PT_WUAREA, pid, (t - uarea_address) + off, data);
	}
}

backtr(link, cnt)
{
	register long	rtn, p, inst;
	long		calladr, entadr;
	int		i, argn;

	while(cnt--)
	{
		p = link; calladr = -1; entadr = -1;
		link = fetch(p, DSP, 4); p += 4;
		if (link == 0)
		{
			printf("no subroutine on stack\n");
			break;
		}
		rtn = fetch(p, DSP, 4);
		if (fetch(rtn - 6, ISP, 2) == CALL6)
		{
			entadr = fetch(rtn - 4, ISP, 4);
			calladr = rtn - 6;
		}
		else if (fetch(rtn - 6, ISP, 2) == LBSR)
		{	calladr = rtn - 6;
			entadr = fetch(rtn-4,ISP,4) + rtn - 4;
		}
		else if (fetch(rtn - 4, ISP, 2) == lBSR)
		{	calladr = rtn - 4;
			entadr = (short)(fetch(rtn-2,ISP,2)) + rtn - 2;
		}
		else if ((fetch(rtn - 2, ISP, 2) & ~0xffL) == lBSR)
		{	calladr = rtn - 2;
			entadr = (char)(fetch(rtn-2,ISP,2)) + rtn;
		}
		else if (fetch(rtn - 2, ISP, 2) == CALL2)
			calladr = rtn - 2;

		inst = fetch(rtn, ISP, 2);
		if ((inst & 0xF13F) == ADDQL)
		{
			argn = (inst>>9) & 07;
			if (argn == 0) argn = 8;
		}
		else if ((inst & 0xFEFF) == ADDL)
			argn = fetch(rtn + 2, ISP, inst & 0x100 ? 4 : 2);
 		else if (inst == LEA)
 			argn = fetch(rtn + 2, ISP, 2);
		else argn = 0;
		if (argn && (argn % 4)) argn = (argn/4) + 1;
		else argn /= 4;

		if (calladr != -1) psymoff(calladr, ISYM, ":");
		else printf("???:");
		while (charpos() % 8) printc(' ');
		if (charpos() == 8) printc('\t');
		if (entadr != -1) psymoff(entadr, ISYM, "");
		else printf("???");
		while (charpos() % 8) printc(' '); printc('(');
		if (argn) printf("%X", fetch(p += 4, DSP, 4)); 
		for(i = 1; i < argn; i++)
			printf(", %X", fetch(p += 4, DSP, 4));
		printf(")\n");
		if (entadr == mainsval) break;
	}
}

fetch(adr, space, size)
{
	register long data;

	data = getword(adr, space);
	if (size != 4) 
	{	data >>= 16;
		data &= 0xFFFF;
	}
	return(data);
}

