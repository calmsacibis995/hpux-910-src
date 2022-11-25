/* @(#) $Revision: 66.1 $ */      
/****************************************
 * C Shell
 ****************************************/

#include "sh.h"

/* Actually uses units in HZ, which may not be 60 */

/*  Called by:
	ptimes ()
*/
/**********************************************************************/
p60ths(l)
	long l;
/**********************************************************************/
{

	l += HZ/20;
	printf("%d.%d", (int) (l / HZ), (int) ((l % HZ) / (HZ/10)));
}

/*  Called by:
	ptimes ()
*/
/**********************************************************************/
psecs(l)
	long l;
/**********************************************************************/
{
	register int i;

	i = l / 3600;
	if (i) {
		printf("%d:", i);
		i = l % 3600;
		p2dig(i / 60);
		goto minsec;
	}
	i = l;
	printf("%d", i / 60);
minsec:
	i %= 60;
	printf(":");
	p2dig(i);
}

/*  Called by:
	psecs ()
*/
/**********************************************************************/
p2dig(i)
	register int i;
/**********************************************************************/
{

	printf("%d%d", i / 10, i % 10);
}

char	linbuf[128];
char	*linp = linbuf;

/*  Called by:
	printf ()
	_p_emit ()
	printprompt ()
	echo ()
	putchar ()
	plist ()
	print_by_column ()
	CharAppend ()
	tenex ()
*/
/**********************************************************************/
putchar(c)
	register int c;
/**********************************************************************/
{

/*	if ((c & QUOTE) == 0 && (c == 0177 || c < ' ' && c != '\t' && c != '\n')) { */
	/* NLS: changed to eliminate CR from being printed as ctrl code */
	/* since putchar no longer strips the 8th bit */
	if ((c & QUOTE) == 0 && (c == 0177 || c < ' ' && c != '\t' && c != '\n' && c != '\015')) {
		putchar('^');
		if (c == 0177)
			c = '?';
		else
			c |= 'A' - 1;
	}
	c &= TRIM;
	*linp++ = c;
	if (c == '\n' || linp >= &linbuf[sizeof linbuf - 2])
		flush();
}

/*  Called by:
	pintr1 ()
*/
/**********************************************************************/
draino()
/**********************************************************************/
{

	linp = linbuf;
}

/*  Called by:
	putchar ()
	process ()
	printprompt ()
	error ()
	bferr ()
	xechoit ()
	search ()
	doglob ()
	echo ()
	pchild ()
	pfork ()
	tenex ()
	backspace ()
*/
/**********************************************************************/
flush()
/**********************************************************************/
{
	register int unit;
#ifdef LFLUSHO
	int lmode = 0;
#endif
	int nwrite;
	int ohaderr;
#include <sys/ioctl.h>


	if (linp == linbuf)
		return;

	if (haderr)
		unit = didfds ? 2 : SHDIAG;
	else
		unit = didfds ? 1 : SHOUT;

	/* Store the number of bytes to be written in nwrite and then */
	/* reset linp so that if the below write hangs, and the user  */
	/* interrupts out, flush() won't keep retrying. Also set      */
	/* haderr and reset it once the write succeeds.               */

	nwrite  = linp - linbuf;
	linp    = linbuf;
	ohaderr = haderr;
	haderr = 1;
#ifdef LFLUSHO
	if (didfds==0 && ioctl(unit, TIOCLGET, &lmode)==0 &&
	    lmode & LFLUSHO) {
		lmode = LFLUSHO;
		ioctl(unit, TIOCLBIC, &lmode);
		write(unit, "\n", 1);
	}
#endif
	write(unit, linbuf, nwrite);
	haderr = ohaderr;
}

/*  Called by:
	prvars ()
	doalias ()
*/
/**********************************************************************/
plist(vp)
	register struct varent *vp;
/**********************************************************************/
{

	if (setintr)
		sigrelse(SIGINT);
	for (vp = vp->link; vp != 0; vp = vp->link) {
		int len = blklen(vp->vec);

		printf(to_char(vp->name));
		printf("\t");
		if (len != 1)
			putchar('(');
		blkpr(vp->vec);
		if (len != 1)
			putchar(')');
		printf("\n");
	}
	if (setintr)
		sigrelse(SIGINT);
}
