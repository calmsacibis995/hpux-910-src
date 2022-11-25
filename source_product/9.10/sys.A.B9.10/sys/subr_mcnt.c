/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/subr_mcnt.c,v $
 * $Revision: 1.4.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:21:17 $
 */
/* HPUX_ID: @(#)subr_mcnt.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


#ifdef GPROF
#include "../h/gprof.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"

/*
 * Froms is actually a bunch of unsigned shorts indexing tos
 */
int profiling = 3;
u_short *froms;
struct tostruct *tos = 0;
long tolimit = 0;
char	*s_lowpc = (char *)0x400;
extern char etext;
char *s_highpc = &etext;
u_long	s_textsize = 0;
int ssiz;
u_short	*sbuf;
u_short	*kcount;

kmstartup()
{
	u_long	fromssize, tossize;

	/*
	 *	round lowpc and highpc to multiples of the density we're using
	 *	so the rest of the scaling (here and in gprof) stays in ints.
	 */
	s_lowpc = (char *)
	    ROUNDDOWN((unsigned)s_lowpc, HISTFRACTION*sizeof(HISTCOUNTER));
	s_highpc = (char *)
	    ROUNDUP((unsigned)s_highpc, HISTFRACTION*sizeof(HISTCOUNTER));
	s_textsize = s_highpc - s_lowpc;
	printf("Profiling kernel, s_textsize=%d [%x..%x]\n",
		s_textsize, s_lowpc, s_highpc);
	ssiz = (s_textsize / HISTFRACTION) + sizeof(struct phdr);

	/* sbuf = (u_short *)wmemall(memall, ssiz); */

	printf("Allocating %d bytes for sbuf.\n", ssiz);
	sbuf = (u_short *)kmem_alloc(ssiz);
	if (sbuf == 0) {
		printf("sbuf alloc failed. No space for monitor buffer(s)\n");
		return;
	}
	blkclr((caddr_t)sbuf, ssiz);

	fromssize = s_textsize / HASHFRACTION;
	printf("Allocating %d bytes for froms.\n", fromssize);
	froms = (u_short *)kmem_alloc(fromssize);
	if (froms == 0) {
		printf("froms alloc failed. No space for monitor buffer(s)\n");
		kmem_free(sbuf, ssiz);
		sbuf = 0;
		return;
	}
	blkclr((caddr_t)froms, fromssize);

	tolimit = s_textsize * ARCDENSITY / 100;
	if (tolimit < MINARCS) 
		tolimit = MINARCS;
	tossize = tolimit * sizeof(struct tostruct);
	printf("Allocating %d bytes for tos.\n", tossize);
	tos = (struct tostruct *)kmem_alloc(tossize);
	if (tos == 0) {
		printf("tos alloc failed. No space for monitor buffer(s)\n");
		kmem_free(sbuf, ssiz);
		sbuf = 0;
		kmem_free(froms, fromssize);
		froms = 0;
		return;
	}
	blkclr((caddr_t)tos, tossize);
	tos[0].link = 0;
		
	((struct phdr *)sbuf)->lpc = s_lowpc;
	((struct phdr *)sbuf)->hpc = s_highpc;
	((struct phdr *)sbuf)->ncnt = ssiz;
	kcount = (u_short *)(((int)sbuf) + sizeof(struct phdr));
#ifdef notdef
	/*
	 *	profiling is what mcount checks to see if
	 *	all the data structures are ready!!!
	 */
	profiling = 0;		/* patch by hand when you're ready */
#endif
}

#ifdef notdef
/* system call to control kernel profiling
 * 
 *    kern_prof (operation, buf_ptr)
 *
 *    operation:  GPROF_INIT      - re-initialize kcount array
 *		  GPROF_ON        - start profiling
 *		  GPROF_OFF       - stop profiling
 *		  GPROF_READ_HDR  - copy sbuf header through user's buf_ptr
 *		  GPROF_READ_DATA - copy kcount array through user's buf_ptr
 */

kern_prof()
{
	struct a {
		int	operation;
		caddr_t	buf_ptr;
	} *uap = (struct a *)u.u_ap;

	if (kcount == NULL)
	{
		u.u_error = ENOMEM;  /* wasn't enough memory to initialize */
		return;
	}

	switch (uap->operation)
	{
		case GPROF_INIT:
			((struct phdr *)sbuf)->idle_count = 0;
			((struct phdr *)sbuf)->user_code_count = 0;
			((struct phdr *)sbuf)->count_overflows = 0;
			blkclr(((caddr_t)sbuf) + sizeof(struct phdr),
			       s_textsize / HISTFRACTION);
			return;

		case GPROF_ON:
			profiling = 0; /* turn it on */
			return;

		case GPROF_OFF:
			profiling = 3; /* turn it off */
			return;

		case GPROF_READ_HDR:
			u.u_error = copyout (sbuf, uap->buf_ptr,
					     sizeof(struct phdr));
			return;

		case GPROF_READ_DATA:
			u.u_error = copyout (kcount, uap->buf_ptr,
					     s_textsize / HISTFRACTION);
			return;

		default:
			u.u_error = EINVAL;
	}
}
#endif

mcount()
{
	register char			*selfpc;	/* A5 */
	register unsigned short		*frompcindex;	/* A4 */
	register struct tostruct	*top;		/* A3 */
	register struct tostruct	*prevtop;	/* A2 */
	register long			toindex;	/* D7 */
	static int s;

#ifdef	lint
	selfpc = (char *)0;
	frompcindex = 0;
#else	lint
	/*
	 *	find the return address for mcount,
	 *	and the return address for mcount's caller.
	 */
	asm("	mov.l	 4(%a6),%a5");	/* selfpc = ... */
	asm("	mov.l	 8(%a6),%a4");	/* frompcindex = ... */
#endif	lint
	/*
	 *	check that we are profiling
	 */
	if (profiling) goto out;
	/*
	 *	insure that we cannot be recursively invoked.
	 *	use special routines so we can profile spl routines
	 */
	s = mcount_CRIT();
	/*
	 *	check that frompcindex is a reasonable pc value.
	 *	for example:	signal catchers get called from the stack,
	 *			not from text space.  too bad.
	 */
	frompcindex = (unsigned short *)((long)frompcindex - (long)s_lowpc);
	if ((unsigned long)frompcindex > s_textsize) goto done;
	frompcindex =
	    &froms[((long)frompcindex) / (HASHFRACTION * sizeof(*froms))];
	toindex = *frompcindex;
	if (toindex == 0) {
		/*
		 *	first time traversing this arc
		 */
		toindex = ++tos[0].link;
		if (toindex >= tolimit) goto overflow;
		*frompcindex = toindex;
		top = &tos[toindex];
		top->selfpc = selfpc;
		top->count = 1;
		top->link = 0;
		goto done;
	}
	top = &tos[toindex];
	if (top->selfpc == selfpc) {
		/*
		 *	arc at front of chain; usual case.
		 */
		top->count++;
		goto done;
	}
	/*
	 *	have to go looking down chain for it.
	 *	top points to what we are looking at,
	 *	prevtop points to previous top.
	 *	we know it is not at the head of the chain.
	 */
	for (; /* goto done */; ) {
		if (top->link == 0) {
			/*
			 *	top is end of the chain and none of the chain
			 *	had top->selfpc == selfpc.
			 *	so we allocate a new tostruct
			 *	and link it to the head of the chain.
			 */
			toindex = ++tos[0].link;
			if (toindex >= tolimit) goto overflow;
			top = &tos[toindex];
			top->selfpc = selfpc;
			top->count = 1;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}
		/*
		 *	otherwise, check the next arc on the chain.
		 */
		prevtop = top;
		top = &tos[top->link];
		if (top->selfpc == selfpc) {
			/*
			 *	there it is.
			 *	increment its count
			 *	move it to the head of the chain.
			 */
			top->count++;
			toindex = prevtop->link;
			prevtop->link = top->link;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}

	}
done:
	mcount_UNCRIT(s);
	/* and fall through */
out:
	return;

overflow:
	profiling = 3;
	printf("mcount: tos overflow\n");
	mcount_UNCRIT(s);
	goto out;
}
#endif	GPROF
