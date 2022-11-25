/* @(#) $Revision: 70.2 $ */

#include "Ktty.h"
#include "basic.h"
#ifdef CDBKDB
#include <stdio.h>
#include <sys/termio.h>
#include <sys/ioctl.h>
#endif /* CDBKDB */

char *kdbbp;
char kdb_interrupt;

export int scroll;			/* scroll if != 0 */
int nslines;				/* number of lines before scroll */

#define MAX_STACK 20
char cmdlines[MAX_STACK][MAX_KDBIN] = {
	"", "", "", "", "",
	"", "", "", "", "",
	"", "", "", "", "",
	"", "", "", "", "",
	};
int cur_line = 0;
int cur_oline = MAX_STACK-1;
int cmd_line = 0;

int cur_virt;   /*cursor position. (Virtual, Physical)*/
#define STOPS	4
short windstops[STOPS] = { 1, 37, 73, 109};
#define SCREENWIDTH (80-4)  /*Screen is assumed to be 80.  And we need some of
				them for prompt, >, < etc.*/
short *wind_beginp = &windstops[0];
short wind_end = 1 + SCREENWIDTH;
#define APPEND	0
#define CNTL	1
#define ESC	'\033'

#define	FIFO_ENABLE	1

#define printable(c) ((c) >= ' ' && (c) <= '~')
#define whitespace(c) ((c)==' ' || (c)=='\t')

kdbttyinit(tp)
register struct kdbtty *tp;
{
	int i;

	if (tp->kdbt_type == APOLLO_UART) {
		struct apollo_pci *pci;

		pci = (struct apollo_pci *) tp->kdbt_addr;

		/* flush the fifo */
		pci->pciistat = 0x87;
		pci->pciistat = FIFO_ENABLE;

		pci->pcilcntl = DLAB;	/* access the baud registers */
		i = KDBBAUD_RATE_APCI;
		pci->baudhi = i>>8;
		pci->baudlo = i & 0xff;
		pci->pcilcntl = KDB_LINECNTL;	/* line control */
		pci->pcilstat = 0;		/* clear line status */
		pci->pciicntl = 0;		/* no interrupts */

	} else {
		struct kdbpci *pci;

		pci = (struct kdbpci *) tp->kdbt_addr;

		/* reset intfc, clear UART */
		pci->pci_id = 0x00;
		/* Wait at least 50 microseconds. */
#ifndef CDBKDB
		for (i=0; i < 2000; i++) {
			kdb_purge();
		}
#endif
		pci->pcilcntl = DLAB;	/* access the baud registers */
		i = KDBBAUD_RATE;
		pci->baudhi = i>>8;
		pci->baudlo = i & 0xff;
		pci->pcilcntl = KDB_LINECNTL;	/* line control */
		pci->pcilstat = 0;		/* clear line status */
		pci->pciicntl = 0;		/* no interrupts */
	}
}

char *
kdbgetline()
{
	register struct kdbtty *tp = &kdb_tty;

	nslines = 0;
	kdbbp = tp->kdb_tail = tp->kdb_buffer;
	kaogetline();
	strcpy(tp->kdb_buffer, cmdlines[cur_line]);
	strout("\r\n");
	if (*cmdlines[cur_line] != '\0') {
		/*
		 * if command line entered has more then 2 char and
		 * different from previous command stored in history,
		 * then enter the new one in history.
		 */
		if (strlen(cmdlines[cur_line]) > 2 &&
		    strcmp(cmdlines[cur_line], cmdlines[cur_oline])) {
			cur_oline = cur_line;
			if (++cur_line >= MAX_STACK)
				cur_line = 0;
		}
		*cmdlines[cur_line] = '\0';
		cmd_line = cur_line;
	}
	return(tp->kdb_buffer);
}

static int cc;

kdbputchar(c)
register unsigned short c;
{
	if (c == 0) return;
	if (c == '\t') {
		do {
			kdbpciout(' ');
		} while (++cc & 07);
		return;
	}
	cc++;
	switch (c) {
	case LF:
		nslines++;
		cc = 0;
		kdbpciout(CR);
		/* Fall through */
	case CR:
	case BELL:
		kdbpciout(c);
		break;
	default:
		show_char(c);
		break;
	}
	if (kdb_interrupt) {
		kdb_interrupt = false;
		cc = 0;
		UError("");
	}
	if (scroll && nslines == 23) {
		strout("--More--");
		c = kdbpciin();
		strout("\r\033K");		/* Erase this line */
		switch (c) {
		case '\r': nslines--; break;	/* one more line */
		case '\004': nslines/=2; break;	/* half a screen */
		default:
		case ' ': nslines=0; break;	/* entire screen */
		}
	}
}



int pci_checked = 0;

char pci_check()
{
	register struct kdbtty *tp = &kdb_tty;
	char c;

	if (c=hil_read())
		return c;

	if (tp->kdbt_type == APOLLO_UART) {
		struct apollo_pci *pci;

		pci = (struct apollo_pci *) tp->kdbt_addr;

		if (pci->pcilstat & RXRDY) {
			pci_checked = 1;
			c = kdbpciin();
		}
	}
	else {
		struct kdbpci *pci;

		pci = (struct kdbpci *) tp->kdbt_addr;

		if (pci->pcilstat & RXRDY) {
			pci_checked = 1;
			c = kdbpciin();
		}
	}
	return c;
}


kdbpciin()
{
#ifdef CDBKDB
	char c;
	static struct termio termio, old_termio;
	static int initialized=false;

	fflush(stdout);				/* write pending output */
	if (!initialized) {
		ioctl(0, TCGETA, &termio);	/* get current term values */
		old_termio = termio;		/* save them */
		termio.c_cc[4] = 1;		/* MIN=1 */
		termio.c_lflag &= ~(ISIG | ICANON | ECHO);
		ioctl(0, TCSETAW, &termio);	/* set new term values */
		initialized=true;
	}
	c=0;					/* Default if read fails */
	read(0, &c, sizeof(c));			/* read a character */
	if (c=='\n')
		c='\r';

	if (c==CNTL_C) {
		ioctl(0, TCSETAW, &old_termio);	/* restore old values */
		exit(1);
	}
#else
	register struct kdbtty *tp = &kdb_tty;
	register unsigned short c;

again:
	if (c=hil_read())
		return c;

	if (tp->kdbt_type == APOLLO_UART) {
		struct apollo_pci *pci;

		pci = (struct apollo_pci *) tp->kdbt_addr;
		if (!pci_checked && !(pci->pcilstat & RXRDY))
			goto again;

		c = pci->pci_data & 0x7f;
	} else {
		struct kdbpci *pci;

		pci = (struct kdbpci *) tp->kdbt_addr;
		if (!pci_checked && !(pci->pcilstat & RXRDY))
			goto again;

		c = pci->pci_data & 0x7f;
	}
	pci_checked = 0;

	if (c == DC1)
		tp->kdbt_flags &= ~WAIT_FOR_DC1;
	else if (c == DC3) {
		tp->kdbt_flags |= WAIT_FOR_DC1;
		goto again;
	}
	else if (c == CNTL_C) {
		tp->kdbt_flags &= ~WAIT_FOR_DC1;
		kdb_interrupt = true;
	}
#endif
	return c;
}


kdbpciout(c)
register unsigned short c;
{
#ifdef CDBKDB
	char ch;
	ch = c;
	fflush(stdout);		    /* write pending output */
	write(1, &ch, sizeof(ch));  /* We don't need no steenking efficiency! */
#else

	register struct kdbtty *tp = &kdb_tty;

	graphics_out(c);

	while (tp->kdbt_flags & WAIT_FOR_DC1)
		kdbpciin();

	if (tp->kdbt_type == APOLLO_UART) {
		struct apollo_pci *pci;

		pci = (struct apollo_pci *) tp->kdbt_addr;

		/* Wait for Transmitter Holding Register to Empty. */

		while (((pci->pcilstat&THRE)==0)&&((pci->pcilstat&THRE_SHFT) == 0));

		pci->pci_data = c;

		/*  Just for fun, check and see if any input -- could be DC1 */
		if (pci->pcilstat & RXRDY)
			kdbpciin();
	} else {
		struct kdbpci *pci;

		pci = (struct kdbpci *) tp->kdbt_addr;

		/* Wait for Transmitter Holding Register to Empty. */

		while ((pci->pcilstat&THRE)==0) ;

		pci->pci_data = c;

		/*  Just for fun, check and see if any input -- could be DC1 */
		if (pci->pcilstat & RXRDY)
			kdbpciin();
	}
#endif
}

beep()
{
	kdbputchar('\07');
}

refresh(mode)
register int mode;
{
	register short *wbp, we;
	register int i;
	register char d, *cp;

	if (mode == APPEND) {
		cp = cmdlines[cur_line] + cur_virt - 1;
		while (*cp++) cur_virt++;
	}

	wbp = wind_beginp;
	we = *wbp + SCREENWIDTH;
	while (1) {
		if (cur_virt > we) {
			if (wbp == &windstops[STOPS-1]) {
				beep();
				printf("refresh: window at the end of line already\n");
				break;
			}
			else {
				wbp++;
				we += SCREENWIDTH;
			}
		}
		else if (cur_virt < *wbp) {
			if (wbp == &windstops[0]) {
				beep();
				printf("refresh: window at the beginning of line already\n");
				break;
			}
			else {
				wbp--;
				we -= SCREENWIDTH;
			}
		}
		else
			break;
	}

	/* Redraw the line */
	kdbpciout('\r');
	if (*wbp == windstops[0])
		d = '>';
	else
		d = '<';
	kdbpciout(d);
	cp = cmdlines[cur_line];
	cp += *wbp;
	cp--;
	for (i=0; i<SCREENWIDTH && *cp; i++, cp++)
		show_char(*cp);
	if (*cp) {
		kdbpciout('>');
		i++;
	}
	strout("\033K");		/* Clear to end-of-line */

	/* Now, go put the cursor somewhere */
	if (mode == CNTL)  {
		kdbpciout('\r');
		kdbpciout(d);
		cp = cmdlines[cur_line];
		for (i = cur_virt - *wbp; i>0; i--, cp++)
			show_char(*cp);
	}
	wind_beginp=wbp;
	wind_end=we;
}


kaogetline()
{
	register char c;
	register char *cp1;
	register int escret;

	cur_virt=1;
	cp1=cmdlines[cur_line];

	while ((c=kdbpciin()) != CR) {
		switch(c) {
		case ESC:
			escret=esccmd();
			checkwind(APPEND);
			if (escret == CR)
				return;
			/* Get cp1 & cur_virt pointing to end of string */
			cur_virt=1;
			cp1=cmdlines[cur_line];
			while (*cp1) {
				cp1++;
				cur_virt++;
			}
			break;
		case BS:
			if (cur_virt == 1) {
				beep();
				break;
			}
			strout("\b \b");

			cp1--;
			if (!printable(*cp1))
				strout("\b \b");
			*cp1='\0';
			cur_virt--;
			checkwind(APPEND);
			break;
		default:
			*cp1++ = c;
			*cp1='\0';
			cur_virt++;
			kdbputchar(c);
			checkwind(APPEND);
		}
	}
}

checkwind(mode)
int mode;
{
	if (cur_virt > wind_end || cur_virt < *wind_beginp)
		refresh(mode);
}


esccmd()
{
	register char c, d, *cp1, *cp2, *cp3;
	char pattern[80];
	int starting_point, patlen;

	if (cur_virt > 1) {
		cur_virt--;
		refresh(CNTL);
	}

	while(1) {
		c=kdbpciin();
		cp1 = cmdlines[cur_line];
		switch (c) {
		case CR:
			return(CR);
		case '-':
		case 'k':
			if (--cmd_line<0)
				cmd_line = MAX_STACK-1;
			cur_virt=1;
			strcpy(cp1, cmdlines[cmd_line]);
			break;
		case '+':
		case 'j':
			if (++cmd_line >= MAX_STACK)
				cmd_line = 0;
			cur_virt=1;
			strcpy(cp1, cmdlines[cmd_line]);
			break;
		case '/':
			/* Read the pattern */
			patlen = 0;
			strout("\r\033K/");
			for (;;) {
				c = kdbpciin();
				if (c==CR)
					break;
				else if (c==BS) {
					if (patlen>0) {
					    strout("\b \b");
					    if (!printable(pattern[patlen-1]))
						strout("\b \b");
					}
				}
				else {
					show_char(c);
					pattern[patlen++] = c;
				}
			}
			pattern[patlen] = '\0';
		
			starting_point = cmd_line;
			for (;;) {
				if (--cmd_line<0)
					cmd_line = MAX_STACK-1;
				if (cmd_line==starting_point)
					break;
				if (match(cmdlines[cmd_line], pattern)) {
					cur_virt=1;
					strcpy(cp1, cmdlines[cmd_line]);
					break;
				}
			}
			break;
		case ' ':
		case 'l':
			if (cp1[cur_virt] == '\0')
				goto error;
			cur_virt++;
			break;
		case 'B':
		case 'b':
			if (cur_virt==1)
				goto error;
			cur_virt--;
			while (cur_virt>1 && !(whitespace(cp1[cur_virt-2]) && !whitespace(cp1[cur_virt-1])))
				cur_virt--;
			break;
		case 'W':
		case 'w':
			if (cp1[cur_virt] == '\0')
				goto error;
			cur_virt++;
			while (cp1[cur_virt] && !(whitespace(cp1[cur_virt-2]) && !whitespace(cp1[cur_virt-1])))
				cur_virt++;
			break;
		case '$':
			cur_virt=1;
			while (cp1[cur_virt])
				cur_virt++;
			break;
		case 'D':
			cp1[cur_virt-1] = '\0';
			if (cur_virt>1)
				cur_virt--;
			break;
		case BS:
		case 'h':
			if (cur_virt == 1)
				goto error;
			cur_virt--;
			break;
		case '^':
			cur_virt = 1;
			while (cp1[cur_virt] && whitespace(cp1[cur_virt-1]))
				cur_virt++;
			break;
		case '0':
			cur_virt = 1;
			break;
		case '~':
			c = cp1[cur_virt-1];
			if ('a' <= c && c <= 'z' || 'A' <= c && c <= 'Z')
				c ^= 'a'^'A';
			cp1[cur_virt-1] = c;

			if (cp1[cur_virt])
				cur_virt++;
			break;
		case 'X':
			if (cur_virt == 1)
				goto error;
			cur_virt--;
			/* Fall through */
		case 'x':
			cp1 += (cur_virt - 1);
			cp2 = cp1;
			if (*cp2 == '\0')
				goto error;
			while (*cp2 = *(cp2+1)) cp2++;
			if (*cp1 == '\0') {
				if (cur_virt > 1) {
					cp1--;
					cur_virt--;
				}
			}
			break;
		case 'r':
			cp1 += (cur_virt - 1);
			d = kdbpciin();
			if (*cp1 == '\0')
				goto error;
			*cp1 = d;
			break;
		case 'I':
			cur_virt = 1;
			refresh(CNTL);
			goto insert;
		case 'C':
			cp1[cur_virt-1] = '\0';
			cur_virt = 1;
			/* Fall through */
		case 'A':
			while (cp1[cur_virt])
				cur_virt++;
			/* Fall through */
		case 'a':
			if (*cp1) cur_virt++;
			refresh(CNTL);
			if (cp1[cur_virt]=='\0')    /* end of line? */
				return 0;	    /* back to normal mode */
			/* Fall through */
		case 'i':
insert:			cp1 += (cur_virt - 1);
			cp3 = cp1;
			while (((d = kdbpciin()) != ESC) && (d != CR)) {
				if (d != BS) {
					cp2 = cp1;
					while (*cp1) cp1++;
					while (cp1 >= cp2) {
						*(cp1+1) = *cp1;
						cp1--;
					}
					*++cp1 = d;
					cp1++;
					cur_virt++;
				}
				else {
					if (cp1 == cp3)
						beep();
					else {
						cp2 = cp1;
						while (*(cp1-1) = *cp1) cp1++;
						cp1 = cp2 - 1;
						cur_virt--;
					}
				}
				refresh(CNTL);
			}
			if (d == CR)
				return(CR);
			cur_virt--;
			break;
		case '':
			showstack();
			break;
		default:
error:			beep();
			break;
		}
		refresh(CNTL);
	}
}

showstack()
{
	register int i;

	for (i=0; i<MAX_STACK; i++) 
		printf("%c %s\n", i==cur_line ? '*' : '-', cmdlines[i]);
}


match(line, pattern)
char *line, *pattern;
{
	int patlen = strlen(pattern);

	for (; *line; line++) {
		if (strncmp(line, pattern, patlen)==0)
			return 1;	/* success */
	}
	return 0;		/* failure */
}


show_char(c)
char c;
{
	if (printable(c))
		kdbpciout(c);
	else {
		kdbpciout('^');
		if (c>'~')
			kdbpciout('?');
		else
			kdbpciout(c+'@');
	}
}

strout(s)
register char *s;
{
	while (*s)
		kdbpciout(*s++);
}
