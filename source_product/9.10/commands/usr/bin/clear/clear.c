
#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 63.1 $";
#endif
/* load me with -lcurses */
/* #include <retrofit.h> on version 6 */
/*
 * clear - clear the screen
 */

#include <stdio.h>
#ifndef hpux
#include <sgtty.h>
#else
#include <curses.h>
#endif hpux

char	*getenv();
char	*tgetstr();
#ifndef hpux
char	PC;
short	ospeed;
#endif hpux
#undef	putchar
int	putchar();

main()
{
	char *cp = getenv("TERM");
	char *clear_str;
#ifndef hpux
	char clbuf[20];
	char *clbp = clbuf;
	char buf[1024];
	char pcbuf[20];
	char *pcbp = pcbuf;
	char *pc;
	struct sgttyb tty;

	gtty(1, &tty);
	ospeed = tty.sg_ospeed;
#endif hpux
	if (cp == (char *) 0)
		exit(1);
#ifndef hpux
	if (tgetent(buf, cp) != 1)
		exit(1);
	pc = tgetstr("pc", (char *) 0);
	if (pc)
		PC = *pc;
#else hpux
	if (tgetent((char *) 0, cp) != 1)
		exit(1);
#endif hpux
	clear_str = tgetstr("cl", (char *) 0);
	if (clear_str)
		tputs(clear_str, tgetnum("li"), putchar);
	exit (clear_str == (char *) 0);
}
