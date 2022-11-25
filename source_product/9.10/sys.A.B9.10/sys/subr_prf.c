/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/subr_prf.c,v $
 * $Revision: 1.7.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 19:31:29 $
 */
/* HPUX_ID: @(#)subr_prf.c	55.1		88/12/23 */

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


#include "../h/debug.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/reboot.h"
#include "../h/vm.h"
#include "../h/msgbuf.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/tty.h"
#include "../s200io/dma.h"
#include "../wsio/iobuf.h"
#include "../wsio/hpibio.h"
#include "../dux/cct.h"
int sysdebug_savecore = 0;


/*
 * Assert failed--bomb out
 */
assfail(a, f, l)
    char *a, *f;
    int l;
{
    printf("assertion failed: %s, file %s, line %d\n", a, f, l);
    panic("assertion error");
}
/*
 * In case console is off,
 * panicstr contains argument to last
 * call to panic.
 */
char	*panicstr;

/*
 * Scaled down version of C Library printf.
 * Used to print diagnostic information directly on console tty.
 * Since it is not interrupt driven, all system activities are
 * suspended.  Printf should not be used for chit-chat.
 *
 * One additional format: %b is supported to decode error registers.
 * Usage is:
 *	printf("reg=%b\n", regval, "<base><arg>*");
 * Where <base> is the output base expressed as a control character,
 * e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the next characters (up to a control character, i.e.
 * a character <= 32), give the name of the register.  Thus
 *	printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * would produce output:
 *	reg=2<BITTWO,BITONE>
 */
/* Added support for width specifiers to support stack dump on panic. */
/* They are used in boot() in machdep.c.                              */
/* Add the following #define to pick up the feature.                  */
#define	WIDTH_SPECIFIER

#ifdef	WIDTH_SPECIFIER
short	printf_pad_size;
char	printf_pad_char;
#endif	WIDTH_SPECIFIER

/*VARARGS1*/
printf(fmt, x1)
	char *fmt;
	unsigned x1;
{
	prf(fmt, &x1, 0, 0, 0);
}

/*
 * msg_printf is used whenever Kernel printout should appear in the
 * messages file indirectly via kernel message buffers rather than
 * printed directly on the console.
 */
/*VARARGS1*/
msg_printf(fmt, x1)
	char *fmt;
	unsigned x1;
{
	prf(fmt, &x1, 1, 0, 0);
}

/*
 * Uprintf prints to the current user's terminal,
 * guarantees not to sleep (so can be called by interrupt routines)
 * and does no watermark checking - (so no verbose messages).
 */
/*VARARGS1*/
uprintf(fmt, x1)
	char *fmt;
	unsigned x1;
{
	prf(fmt, &x1, 2, 0, 0);
}

/*
 * Sprintf does the same formatting as printf, but the result is
 * returned to the caller's buffer.  The second argument is the length
 * of the buffer; formatted strings longer than this are truncated.
 */
/*VARARGS1*/
sprintf(pbuf, len, fmt, x1)
	char *pbuf, *fmt;
	int len;
	unsigned x1;
{

	prf(fmt, &x1, 3, &pbuf, &len);
}

prf(fmt, adx, touser, pbuf, len)
	register char *fmt;
	register u_int *adx;
	char **pbuf;
	int *len;
{
	register int b, c, i;
	char *s;
	int any;

loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0') {
			if (touser == 3)
				xputchar('\0', touser, pbuf, len);
			return;
		}
		xputchar(c, touser, pbuf, len);
	}
#ifdef	WIDTH_SPECIFIER
	printf_pad_size = 0;
	printf_pad_char = '\0';	/* default unless leading zero seen */
#endif	WIDTH_SPECIFIER
again:
	c = *fmt++;
	/* THIS CODE IS VAX DEPENDENT IN HANDLING %l? AND %c */
	switch (c) {

#ifdef	WIDTH_SPECIFIER
	case '0':
		if (printf_pad_char == '\0')
			printf_pad_char = '0';

	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		if (printf_pad_char == '\0')
			printf_pad_char = ' ';
		printf_pad_size = printf_pad_size * 10 + c - '0';
		goto again;
#endif	WIDTH_SPECIFIER

	case 'l':
		goto again;
	case 'x': case 'X':
		b = 16;
		goto number;
	case 'd': case 'D':
	case 'u':		/* what a joke */
		b = 10;
		goto number;
	case 'o': case 'O':
		b = 8;
number:
		printn((u_long)*adx, b, touser, pbuf, len);
		break;
	case 'c':
		b = *adx;
		for (i = 24; i >= 0; i -= 8)
			if (c = (b >> i) & 0x7f)
				xputchar(c, touser, pbuf, len);
		break;
	case 'b':
		b = *adx++;
		s = (char *)*adx;
		printn((u_long)b, *s++, touser, pbuf, len);
		any = 0;
		if (b) {
			xputchar('<', touser, pbuf, len);
			while (i = *s++) {
				if (b & (1 << (i-1))) {
					if (any)
						putchar(',', touser);
					any = 1;
					for (; (c = *s) > 32; s++)
						putchar(c, touser);
				} else
					for (; *s > 32; s++)
						;
			}
			if (any)
				xputchar('>', touser, pbuf, len);
		}
		break;

	case 's':
		s = (char *)*adx;
		while (c = *s++)
			xputchar(c, touser, pbuf, len);
		break;

	case '%':
		xputchar('%', touser, pbuf, len);
		break;
	}
	adx++;
	goto loop;
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernel stacks.
 */
printn(n, b, touser, pbuf, len)
	u_long n;
	char **pbuf;
	int *len;
{
	char prbuf[11];
	register char *cp;

	if (b == 10 && (int)n < 0) {
		xputchar('-', touser, pbuf, len);
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
#ifdef	WIDTH_SPECIFIER
		--printf_pad_size;
#endif	WIDTH_SPECIFIER
	} while (n);
#ifdef	WIDTH_SPECIFIER
	while (printf_pad_size-- > 0)
		xputchar (printf_pad_char, touser, pbuf, len);
#endif	WIDTH_SPECIFIER
	do
		xputchar(*--cp, touser, pbuf, len);
	while (cp > prbuf);
}



int do_savecore = 1; 	/*  adb to 0 to turn off savecore; should  */
			/*  make customer-configurable             */

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then reboots.
 * If we are called twice, then we avoid trying to
 * sync the disks as this often leads to recursive panics.
 */

char panic_msg[255];			/* used in ufs_alloc.c, et.al. */
int  panic_msg_len = sizeof(panic_msg);	/* used in ufs_alloc.c, et.al. */

int s_lev;	/* saved PS level from entry to panic */

panic(s)
	char *s;
{
	extern struct tty *cons_tty;
	extern long dbg_addr;     /* monitor return address */
	extern char ite_bmap_semi_lock;	/* graphics screen lock */
	extern int ite_memory_used;
	extern int static_colormap;
	extern int dumpsize;
	extern dev_t dumpdev;
	extern long dumplo;
	extern u_int my_site_status;
	extern struct proc *cur_proc;
	int bootopt;
	extern int monitor_on;
	char sbuf[50];

	if (!mainentered)
	    cur_proc = (struct proc *)0;
	else {

	    /*
	     * Save the registers so that savecore() can get an idea
	     * of what the current process' stack frame looks like.
	     * This must be first code executed in panic.
	     */
	    s_lev = update_rsave();

	    /* save current process pointer */

	    cur_proc = u.u_procp;
	}

	bootopt = RB_AUTOBOOT;

	if (panicstr) {
		bootopt |= RB_NOSYNC;
		do_savecore = 0;  /*  no matter what - this is 2nd panic  */
	} else {
		panicstr = s;
	}

	/*
	 * A panic while using windows is bad, because you can't
	 * read the panic message.  To cure this, we hard-reset the ITE,
	 * which will clear the screen.  We only do this is somebody
	 * is doing graphics (that is, when /dev/crt is open), because
	 * we don't want to hard reset when the user is just using the
	 * plain ITE without graphics.
	 */
	if (ite_memory_used) {		/* Is somebody using the crt? */
		ite_bmap_semi_lock=0;	/* graphics lock is useless now */
		static_colormap=0;	/* go ahead, trash the color map */
		/* make sure we can see the ITE if windows are running */
		printf("\033E");	/* hard reset */
	}
	if (cons_tty != NULL) {
		printf("panic: %s\n", s);
		printf("Please record the panic message/info above\n");	
	} else if (!monitor_on) {
		strcpy(sbuf,"panic: ");
		strncat(sbuf, s, 42);
		if (!do_savecore)
			rom_printf(sbuf); /* never returns */
	}

	/*
	 * Push out to the debugger in preference to dumping core.  We
	 * use the global variable sysdebug_savecore to tell if we should
	 * go ahead and dump core after returning from SYSDEBUG.
	 */
	if (monitor_on) {
		/* call debugger if it is present */
		asm ("	mov.w	&0,-(%sp) ");
		asm ("	pea	ret_from_debug ");
		asm ("	mov.w	_s_lev,-(%sp) ");
		asm ("	mov.l	_dbg_addr,%a0 ");
		asm ("	jmp	(%a0) ");
		asm ("ret_from_debug: ");
		if (!sysdebug_savecore)
			return;
	}

	if (cons_tty && do_savecore) {
		if (dumpdev == NODEV) {
			printf ("no dump device - can't save corefile\n");
		} else {
			printf ("\nDumping %d bytes to dev 0x%x, offset %d\n",
				dumpsize * NBPG, dumpdev, dumplo);
			printf ("Don't cycle power until core dump completes...\n");
			printf ("If you don't need this core dump, you may now cycle power.\n\n");

			if ((dumpdev = savecore()) >= 0) {
				printf ("Core file was successfully written to device %x\n", dumpdev);
			} else {
				printf ("Dump unsuccessful.\n");
			}
			printf ("Core dump completed, you may now cycle power.\n\n");
		}
	} else if (do_savecore) {
		if (dumpdev != NODEV)
			savecore();	/* no console, have swap device*/
	}

	if ((cons_tty == NULL) && (!monitor_on))
		rom_printf(sbuf); /* never returns */

	bootopt |= RB_PANIC;
	boot(bootopt, &s);	/* pass pointer to stack location to dump */
	for (;;); /* loop forever */
}

/*
 * Warn that a system table is full.
 */
tablefull(tab)
	char *tab;
{

	printf("%s: table is full\n", tab);
}

/*
 * Hard error is the preface to plaintive error messages
 * about failing disk transfers.
 */
harderr(bp, cp)
	struct buf *bp;
	char *cp;
{

	printf("%s%d%c: hard error sn%d ", cp,
	    dkunit(bp), 'a'+(minor(bp->b_dev)&07), bp->b_blkno);

}

/*
 * For sprintf, put the character into the user's buffer,
 * else call putchar.
 */
xputchar(c, touser, pbuf, len)
	register int c;
	char **pbuf;
	int *len;
{
	switch (touser) {
	case 3:				/* sprintf */
		if ((*len) > 0)
			*(*pbuf)++ = ((*len)-- == 1 ? '\0' : c);
		break;

	case 1:				/* msg_printf */
		errorlog_putchar(c);
		break;

	case 0:				/* printf */
		putchar(c, touser);	/* Send it to the console */
		errorlog_putchar(c);	/* and to the message log! */
		break;

	default:			/* Probably 2: uprintf */
		putchar(c, touser);
	}

}

/*
 * Put the character 'c' into the error log buffer.
 */
errorlog_putchar(c)
register c;
{
	if (c==0 || c=='\r' || c==0177)		/* useless stuff? */
		return;				/* forget it */

	/* Initialize buffer if needed */
	if (Msgbuf.msg_magic != MSG_MAGIC) {
		register int i;

		Msgbuf.msg_bufx = 0;
		Msgbuf.msg_magic = MSG_MAGIC;
		for (i=0; i < MSG_BSIZE; i++)
			Msgbuf.msg_bufc[i] = 0;
	}

	if (Msgbuf.msg_bufx < 0 || Msgbuf.msg_bufx >= MSG_BSIZE)
		Msgbuf.msg_bufx = 0;

	Msgbuf.msg_bufc[Msgbuf.msg_bufx++] = c;
}

/*
 *	savecore() attempts to copy an image of all of physical memory 
 *	to the tail end of the swap space on the root volume.  The next
 *	time the system is booted, a user program "savecore(1M)" attempts
 *	to copy it to a named file so the panic can be debugged.
*/

savecore ()
{
	extern int dumpsize, dumplo;
	int s, error;
	unsigned mem_size_bytes;
	register struct isc_table_type *sc = isc_table[m_selcode(dumpdev)];
	struct buf scbuf;		/* Put buf struct on stack */ 
	struct iobuf sciobuf;
	struct buf *bp = &scbuf;

	s = spl6();	/* Turn "off" interrupt system */
/* 
 * "dumpsize" is the number of NBPG-sized pages of memory.  
 * Shift by the page size to get it in bytes
 */
	mem_size_bytes = dumpsize << PGSHIFT;
/*
 * If card is owned then impersonate owner and abort I/O ie clear card
 */
	if (sc->owner != NULL)
		(*sc->iosw->iod_abort)(sc->owner);

	sc->owner = bp;				/* I am now owner */
	sc->transfer = FHS_TFR;			/* Fast Handshake Xfer */
	scbuf.b_flags = 0;
	scbuf.b_ba = m_busaddr (dumpdev);	/* HP-IB bus addr of disk */
	scbuf.b_sc = sc;			/* Select code pointer */
	scbuf.b_queue = &sciobuf;		/* iobuf on stack */
	scbuf.b_bcount = mem_size_bytes;	/* Xfer size */
	scbuf.b_flags = B_WRITE;
	scbuf.b_dev = dumpdev;
	scbuf.b_error = 0;

	sciobuf.timeflag = 0;

	(*sc->iosw->iod_clear)(bp);	/* Clear disk */

/* Call either the cs-80 dump routine or SCSI or AMIGO */

	error = (*bdevsw[major(dumpdev)].d_dump)(bp);

	(*sc->iosw->iod_clear)(bp);	/* Clear disk */
	splx(s);

	return ((error == -1) ? NODEV : dumpdev);
}
