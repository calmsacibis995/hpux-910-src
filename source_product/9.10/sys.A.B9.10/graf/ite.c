/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite.c,v $
 * $Revision: 1.8.84.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/10/11 07:43:38 $
 */

/* HPUX_ID: @(#)ite.c	55.1		88/12/23 */

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

#include "../h/param.h"
#include "../h/tty.h"
#include "../h/ld_dvr.h"
#define _INCLUDE_TERMIO
#include "../h/termios.h"
#include "../h/file.h"
#include "../h/user.h"
#include "../h/systm.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../s200io/bootrom.h"
#include "../h/utsname.h"
#include "../wsio/hpibio.h"
#include "../graf/ite.h"
#include "../graf/kbd.h"
#include "../graf/ite_scroll.h"

/* Temporary until it can be defined in tty.h */
#define T_VHANGUP       -1

/* time for cursor blinking (200) */
#define BLINKSPSEC	5

/* semiphore time in seconds */
#define SEMIPHORE_TIMEOUT 10

#define BLINK_TIME (HZ/BLINKSPSEC)
#define SEMI_TIMEOUT (BLINKSPSEC*SEMIPHORE_TIMEOUT)

/* use the 4.2 version so I can use untimeout */
#undef timeout 

#define ite_tp	ppoll_f

extern int ite_select_code, ite_type;

char ite_bmap_been_locked, ite_bmap_semi_lock;

extern char *user_defaults[][2];
extern int char_switch();

extern int kbd_mute_char(), kbd_command();
extern struct k_keystate k_keystate;

#define TXINTEN 0x02
#define RXINTEN 0x0D
#define SETBRK	0x40

struct iterminal iterminal;

int	ttrstrt();
int	ite_proc();
int	ite_isr();
int	ite_isr_sw();

/* Open the console device.
*/
ite_open(dev, flag)
int dev;
int flag;
{
	struct iterminal *ite = &iterminal;
	register x;
	register struct tty *tp;
	register struct proc *pp;
	int err;
	/* spl must be early so that system won't be in a strange state
	** if an interrupt occurs immediately after the interrupt mask
	** is set in ite_param.
	*/
	
	tp = ite->ite_tty;
	x = splx(tp->t_int_lvl);
	
	if ((tp->t_state&ISOPEN) == 0) {
		/* ttinit(tp); -- not needed for the console device */
		tp->t_proc = ite_proc;
		ite_param();	/* initialize device */
	}
	
	tp->t_state |= CARR_ON;	/* make SURE that CARR_ON is set */
	tp->t_state |= WOPEN;
	
	err = (*linesw[tp->t_line].l_open)(dev, tp);
	
	/* Check to see if this ite should be assigned as the controlling 
	   terminal for this process */
	pp = u.u_procp;
	if (!(flag&FNOCTTY) &&
	    (pp->p_pid == pp->p_pgrp) && (pp->p_pid == pp->p_sid) && 
	    (u.u_procp->p_ttyp == NULL) && (tp->t_cproc == NULL) ) {
		u.u_procp->p_ttyp = tp;
		u.u_procp->p_ttyd = tp->t_dev;
		tp->t_pgrp = pp->p_pgrp;
		tp->t_cproc = pp;
	}

	tp->tty_count++;

        /* Clear the vhangup flag when the file is open */
        ite_proc(tp, T_VHANGUP, 0);

	splx(x);
	return(err);
}

/* Close the console device.
*/

ite_close(dev)
{
/*---------------------------------------------------------------------------
 | FUNCTION -		called on last ite close.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 |  1 May 89  garth	Removed reinitialization of t_pgrp and t_cproc.
 |			They get reset in the line discipline (ttclose).
 |			
 --------------------------------------------------------------------------*/

	struct iterminal *ite = &iterminal;
	struct tty *tp;

	tp = ite->ite_tty;
	if (--tp->tty_count)
		return;		/* only close on last one */
	(*linesw[tp->t_line].l_close)(tp);
}


ite_read(dev, uio)

{
	struct iterminal *ite = &iterminal;
	/* Call the generic TTY read routine */
	ttread (ite->ite_tty, uio);
}


ite_write(dev, uio)

{
	struct iterminal *ite = &iterminal;
	/* Call the generic TTY write routine */
	ttwrite (ite->ite_tty, uio);
}


/* this is to get the tty pointer then call ttselect */
ite_select(dev, rw)
dev_t dev;
{
	struct iterminal *ite = &iterminal;
	ttselect(ite->ite_tty, rw);
}


/* Simply pass the work on to the common
** tty ioctl routines.
*/
ite_ioctl(dev, cmd, arg, mode)
caddr_t arg;
{
	struct iterminal *ite = &iterminal;
	int err;

	if ((err = ttiocom(ite->ite_tty, cmd, arg, mode)) == -1) {
		err = 0;
		ite_param();
	}
	return(err);
}


scroller_mf(a,b)
{
	struct iterminal *ite = &iterminal;
	register struct tty *tp = ite->ite_tty;
	register x = splsx(tp->t_int_lvl);

	a &= 0xff;

	scroller(a,b);
	splsx(x);
}


/*
 * See if we have an ite.  If we do, initialize it.
 *
 * addr=0x560000:	check for internal bit-mapped display
 * addr=0x530000:	check for internal lo-resbit-mapped display
 * addr=0x000000:	check for internal alpha display
 * addr=other:		check at that address for a bit-mapped display
 */
struct tty *
ite_pwr(isc_num,type,addr,id,il,cs,pci_total)
int isc_num;
register id;
char *addr;
int *cs;
int *pci_total;
{
	struct iterminal *ite = &iterminal;
	register struct tty *tp;
	static struct tty ite_tty_struct;
	register struct isc_table_type *isc;
	extern struct tty *cons_tty;
	extern int cons_dev;
	extern struct tty_driver ite_driver;
	extern char ite_present;
	extern char windex_present;
	extern char ttcchar[];
	extern int (*ite_scrl)();

	if (ite_present)	/* check if second pass? */
		return((struct tty *)0);

	/* Make sure there is a keyboard */
	if (sysflags & NOKBD)
		return((struct tty *)0);

	/* Check for alpha display? */
	/*******************************
	if (!testr(0x513001, 1))
		return((struct tty *)0);
	*******************************/

	/* Initialize the internal keyboard/CRT. 	*/

	ite->card = (unsigned char *) addr;	/* NULL for alpha */
	ite_type = type;			/* Which type of card? */

 	if (crt_init(ite) == 0)
		return((struct tty *)0);

	tp = &ite_tty_struct;
	ite->ite_tty = tp;

	tp->t_drvtype = &ite_driver;
	tp->t_card_ptr = addr;

	/* Set up input queue for it */
	tp->t_in_head = tp->t_in_tail = tp->t_inbuf = 
		(ushort *)calloc(TTYBUF_SIZE*2);
	tp->t_in_count = 0;
	tp->t_int_flag = 0;
	tp->t_rcv_flag = FALSE;
	tp->t_xmt_flag = FALSE;
	tp->t_int_lvl = 1;

	/* enable keyboard and RPG to interrupt */
	kbd_init(tp);

	char_switch();

	/* Default values for the console device.
	** Some of these defaults can be changed by 
	** the user through an ioctl system call.
	*/
	tp->t_state = CARR_ON;
	tp->t_iflag = ICRNL;
	tp->t_oflag = OPOST|ONLCR|TAB3;
	tp->t_lflag = ISIG|ICANON|ECHO|ECHOK;
	tp->t_cflag = CLOCAL|B9600|CS8|CREAD;
	tp->t_pgrp = 0;		/* not a controlling terminal */
	tp->t_cproc = NULL;

	/* setup default control characters */
	bcopy(ttcchar, tp->t_cc, NLDCC);

   	ite_param();
	tp->t_sicnl &= ~(TXINTEN|RXINTEN); /* no interrupts until open */

	/* reset terminal configuration to defaults */
	ite->remote_mode = TRUE;
	ite->local_echo = FALSE;
	ite->xmit_fnctn = FALSE;
	ite->insert_mode = FALSE;
	ite->disp_funcs = FALSE;
	ite->escape = FALSE;

	ite_scrl = scroller_mf;
	kbd_beep();

	ite->pstate = 0;	/* Initialize the parser. */

	/* have this be the console until external is found */
	cons_tty = tp;	/* our tty structure */
	cons_dev = 0;	/* doesn't really matter for single-ITE system */

	/* make a selcode entry */
	isc = (struct isc_table_type *)calloc(sizeof(struct isc_table_type));
	isc_table[0] = isc;		/* we are select code zero */
	(struct tty *)isc->ite_tp = tp;	/* store tp in sc */
	(struct tty_driver *)isc->tty_routine = &ite_driver;
	*cs = 1;
	/* tell the graphics driver my select code number */
	ite_select_code = isc_num;

	ite_present = TRUE;
	/* Increment to next location */
	(*pci_total)++;
	return(tp);
}


extern iteputchar();
ite_who_init(tp, tty_num, sc)
{

#ifndef QUIETBOOT
	if (tty_num == -1)
		printf("    System Console is ITE\n");
	else {
		printf("    ITE is tty");
		if (tty_num<10)
			printf("0");
		printf("%d\n",tty_num);
	}
#endif /* ! QUIETBOOT */
}

ite_nop()
{
	return(0);
}

struct tty_driver ite_driver = {
ITE200, ite_open, ite_close, ite_read, ite_write, ite_ioctl, ite_select,
	iteputchar, ite_nop, ite_pwr, ite_who_init, 0
};


ite200_link()
{
	extern struct tty_driver *tty_list;

	ite_driver.next = tty_list;
	tty_list = &ite_driver;
}

/* Set up console device according to the
** ioctl arguments.
*/

ite_param()
{
	struct iterminal *ite = &iterminal;
	register struct tty *tp = ite->ite_tty;

  	/* Neither line control nor modem control make
	** sense for the internal keyboard/crt.
	** Any line/modem control arguments are simply
	** ignored.
	*/
	tp->t_slcnl = 0;
  	tp->t_smcnl = 0;
	if (tp->t_cflag&CREAD)
		tp->t_sicnl |= RXINTEN;
	else
		tp->t_sicnl &= ~RXINTEN;
	tp->t_sicnl |= TXINTEN;
	/* if 1st time start clock */
	if (!tp->t_time)
		ite_isr_sw(ite);
}

ite_proc(tp, cmd, data )
register struct tty *tp;
register int    cmd;
register int    data;
{
	extern int runrun;
	register struct iterminal *ite = &iterminal;
	register struct proc *p = u.u_procp; /* XXX */
	register scrstat;

	switch(cmd) {
	case T_TIME:
		tp->t_state &= ~TIMEOUT;
		tp->t_slcnl &= ~SETBRK;
		goto start;

	case T_WFLUSH:
	case T_RESUME:
		tp->t_state &= ~TTSTOP;
		goto start;

	case T_OUTPUT:
start:
		/* premature (not empty clist) CAN sleep process */
		tp->t_int_flag |= PEND_TXINT;

		/* 
		 * check if someone sent a signal to the process that is
		 * currently not being blocked or ignored. 
		 */
		if (p->p_sig & ~p->p_sigmask & ~p->p_sigignore)
		        break;

		/* check if output is suspended */
		if (tp->t_state&(TIMEOUT|TTSTOP))
			break;

		/* check if in configuration screen */
		if (ite->key_state==TCONFIG || ite->key_state==SET_USER)
			scroller(DSP_LABELS, ite->last_labels, 0);

		while (tp->t_outq.c_cc) {
			/* check if someone clawing for p-reg */
/*****			if (some method of checking for process needs)
				break;
*****/
			/* output another cblock */
			ite_out_cblock(tp);
		}
		/* nothing in queue, DO NOT sleep the process */
		tp->t_int_flag &= ~PEND_TXINT;

		/* check for sleepers to wakeup */
		ttoutwakeup(tp); 
		break;

	case T_SUSPEND:
		tp->t_state |= TTSTOP;
		break;

	case T_BLOCK:
		tp->t_state |= TBLOCK;
		tp->t_sicnl &= ~TXINTEN;
		break;

	case T_RFLUSH:
		if (!(tp->t_state&TBLOCK))
			break;
	case T_UNBLOCK:
		tp->t_state &= ~TBLOCK;
		tp->t_sicnl |= TXINTEN;
		break;

	case T_BREAK:
		tp->t_slcnl |= SETBRK;
		tp->t_state |= TIMEOUT;
		timeout(ttrstrt, tp, HZ>>2);
		break;

	case T_VHANGUP:
               /*
                * If data is non-zero, access to this port is being
                * revoked.  Update the vhangup flag and call the line
                * discipline control routine with LDC_VHANGUP.
                * Since the ite driver itself does not sleep, the only place
                * where access to the ite could sleep is reading from the
                * keyboard, and that will occur within the line discipline,
                * so no local wakeups need be performed here.
                */

                ttcontrol( tp, LDC_VHANGUP, data );
                break;
	}
}

ite_isr(ite)
struct iterminal *ite;
{
	register struct tty *tp = ite->ite_tty;
	sw_trigger(&tp->xmit_intloc, ite_isr_sw, ite, 0, tp->t_int_lvl);
}


/* this routine checks for semiphore timeout, wakes up cus, restores screen */
/* note: this routine is already a sw_trigger level 1 */
ite_isr_sw(ite)
register struct iterminal *ite;
{
	register struct tty *tp = ite->ite_tty;

	tp->t_time = BLINK_TIME;

	/* check if output is suspended */
	if (tp->t_state&(TIMEOUT|TTSTOP)) 
		goto timeout_return;

	/* check if screen needs restoring */
	if (ite->missed_output || tp->t_outq.c_cc) {

		/* get screen accessability */
		switch (check_screen_access(ite)) {

		/* if alpha off, skip */
		case SCR_ALPHAOFF:
			break;

		case SCR_TFORM_BROKE:
			if (ite->flags & RENAISSANCE)
				renais_pwr_reset(ite);
			break;

		case SCR_TFORM_BUSY:
		case SCR_SEMIP_LOCK:
			/* no access, bump timeout counter */
			if (ite->time_out_count++ > SEMI_TIMEOUT) {
				/* clear semiphore or reset transform engine */
				if (ite_bmap_semi_lock)
					ite_bmap_semi_lock=0;
				else
					scroller_init(SOFT);
			} else
				goto timeout_return;
			break;
		case 0:
			if (ite->missed_output) 
				restore_missing_scn(ite);
			break;
		}
		/* reset the semiphore timeout counter */
		ite->time_out_count = 0;

		/* output one cblock if something in queue */
		ite_out_cblock(tp);
	}
	/* this is the wrong way, have ite_proc finish if possible */
 	ttoutwakeup(tp);

	/* get called back in 250 ms */
timeout_return:
	timeout(ite_isr, ite, tp->t_time);
}

/* this routine will output one cblock if any left */
ite_out_cblock(tp)
register struct tty *tp;
{
	extern unsigned char partab[];
	register struct iterminal *ite = &iterminal;
	register int n, flag;
	register unsigned int c;
	register unsigned char *cbuf, *qbp;
	register struct cblock *cp;
	unsigned char qblock[CBSIZE];
	int j;

	flag = ite->dsp_funcs | ite->disp_funcs | ite->pstate;

	/* get the next cblock from the clist */
	if ((cp = getcb(&tp->t_outq)) == NULL)
		return;

	/* if not in remote ignore the request */
	if (!ite->remote_mode) 
		goto ignore;

	/* now empty the cblock */
	n = cp->c_last - cp->c_first;
	cbuf = (unsigned char *)&(cp->c_data[cp->c_first]);
	qbp = qblock;

	/* test if cblock is clean */
	while (n--) {
		if ((partab[c = *cbuf++]&077)||flag) {
			if (j = qbp-qblock) 
				scr_chars_out(qbp = qblock, j);
			crtio(c);
			flag = ite->dsp_funcs | ite->disp_funcs | ite->pstate;
		} else
			*qbp++ = c;
	}
	if (j = qbp-qblock)
		scr_chars_out(qbp = qblock, j);

	/*
	 * put back the cursor if we're not coming back immediately
	 * and not in middle of <esc> sequence.
	 */
	if (tp->t_outq.c_cc==0 && !ite->pstate)
		scroller(SET_CURSOR, ite->xpos, ite->ypos);
ignore:	putcf(cp);
}


ite_filter(key)
register unsigned key;
{
	register struct iterminal *ite = &iterminal;
	register struct k_keystate *k = &k_keystate;
	register struct tty *tp = ite->ite_tty;
	register control, shift;
			
	control = (key&(K_CONTROL_B<<8) ? TRUE : FALSE);
	shift = (key&(K_SHIFT_B<<8) ? TRUE : FALSE);
	key &= ((K_SPECIAL)<<8)+0xFF;
	if (key == K_ILLEGAL) { /* Unexpected key */
		kbd_beep();
		return;
	}

	if (key&(K_SPECIAL<<8)) { 	/* special key */
		register int transmit_fnctn;

		if (ite->xmit_fnctn || ite->dsp_funcs ||
		    (ite->disp_funcs && (ite->key_state != TCONFIG) &&
		    (ite->key_state != SET_USER))) {
			transmit_fnctn = 1;
		} else {
			transmit_fnctn = 0;
		}

		switch(key) {
		case K_KNOB_DOWN+(K_SPECIAL<<8):
			/* Knob Cursor Down Key */
			if (transmit_fnctn)
				send_escape("\033B",2);
			else
				scroller(CURSOR_DOWN);
			return;
		case K_KNOB_UP+(K_SPECIAL<<8):
			/* Knob Cursor Up Key */
			if (transmit_fnctn)
				send_escape("\033A",2);
			else
				scroller(CURSOR_UP);
			return;
		case K_SELECT: /* select key */
			if (transmit_fnctn)
				send_escape("\033&P",3);
		case K_CAPS_LOCK: /* caps key */
			kbd_beep();
		case K_CAPS_ON:	/* caps key on indicator */
		case K_CAPS_OFF:/* caps key off indicator */
		case K_GO_ROMAN:	/* Go to ROMAN keyboard indicator */
		case K_GO_KATAKANA:	/* Go to KATAKANA keyboard indicator */
			return;
		case K_TAB:	/* tab */
		case (K_NP_TAB&(~(K_NPAD<<8))):	/* tab */
			if (shift) {	/* shifted tab */
				if ((ite->key_state == TCONFIG) ||
					(ite->key_state == SET_USER)) {
					/* back tab */
					scroller(TAB_DIR,BACKWARD);
					return;
				}
				if (transmit_fnctn)
				        send_escape("\033i",2);
				else
					scroller(TAB_DIR,BACKWARD);
				return;
			}
			if ((ite->key_state == TCONFIG) ||
				(ite->key_state == SET_USER)) {
				scroller(TAB_DIR, FORWARD);
				return;
			}
			key = TAB;	/* forward tab */
			break;
		case K_K0:	/* k0 -- (f1 on NIMITZ) */
			softkey_input(ite, 1);
			return;
		case K_K1:	/* k1 -- (f2 on NIMITZ, f1 on ITF150) */
			softkey_input(ite, k->type==K_NIMITZ ? 2 : 1);
			return;
		case K_K2:	/* k2 -- (f3 on NIMITZ, f2 on ITF150) */
			softkey_input(ite, k->type==K_NIMITZ ? 3 : 2);
			return;
		case K_K5:	/* k5 -- f5 */
			softkey_input(ite, 5);
			return;
		case K_K6:	/* k6 -- f6 */
			softkey_input(ite, 6);
			return;
		case K_K7:	/* k7 -- f7 */
			softkey_input(ite, 7);
			return;
		case K_K3:	/* k3 -- (f4 on NIMITZ, f3 on ITF150) */
			softkey_input(ite, k->type==K_NIMITZ ? 4 : 3);
			return;
		case K_K8:	/* k8 -- f8 */
			softkey_input(ite, 8);
			return;
		case K_SYSTEM:
			if (shift)	/* USER Key */
				scroller(DSP_LABELS, USER, 0);
			else	/* SYSTEM Key */
				scroller(DSP_LABELS, AIDS, 0);
			return;
		case K_MENU:	/* Turn labels on/off */
			if (control) {	/* USER Definition Menu */
				scroller(DSP_LABELS, SET_USER, 0);
				return;
			}
			switch (ite->key_state) {
			case SET_USER:
			case TCONFIG:
			case DCONFIG:
				break;
			default:
				if (ite->flags&SFK_ON)
					scroller(DSP_LABELS, CLEAR, 0);
				else
					scroller(DSP_LABELS, ite->key_state, FORCE);
				break;
			}
			return;
		case K_K9:	/* k9 -- (On NIMITZ only: AIDS unshifted
						CLR AIDS shifted */
			if (!shift) {
				scroller(DSP_LABELS, AIDS, 0);
				return;
			}
			/* CLR AIDS */
		case K_K4:	/* k4 -- (CLR AIDS on NIMITZ, f4 on ITF150) */
			if (k->type==K_NIMITZ)
				scroller(DSP_LABELS, CLEAR, 0);
			else
				softkey_input(ite, 4);
			return;
		case K_DOWN_ARROW:	/* down arrow */
			if (shift) { /* shifted down arrow - ROLL DOWN */
				if (transmit_fnctn)
					send_escape("\033T",2);
				else
					scroller(ROLL_DOWN);
			} else {
				if (transmit_fnctn)
					send_escape("\033B",2);
				else
					scroller(DOWN_ARROW);
			}
			return;
		case K_UP_ARROW:	/* up arrow */
			if (shift) { /* shifted up arrow - ROLL UP */
				if (transmit_fnctn)
					send_escape("\033S",2);
				else
					scroller(ROLL_UP);
			} else {
				if (transmit_fnctn)
					send_escape("\033A",2);
				else
					scroller(UP_ARROW);
			}
			return;
		case K_KNOB_LEFT+(K_SPECIAL<<8):
		case K_LEFT_ARROW:	/* left arrow */
			if (shift && k->type==K_NIMITZ) {
				/* shifted left arrow - HOME UP */
				if (transmit_fnctn)
					send_escape("\033h",2);
				else
					scroller(HOME_UP);
			} else {
				if (transmit_fnctn)
					send_escape("\033D",2);
				else
					scroller(CURSOR_LEFT);
			}
			return;
		case K_KNOB_RIGHT+(K_SPECIAL<<8):
		case K_RIGHT_ARROW:	/* right arrow */
			if (shift && k->type==K_NIMITZ) {
				/* shifted right arrow - HOME DOWN */
				if (transmit_fnctn)
					send_escape("\033F",2);
				else
					scroller(HOME_DOWN);
			} else {
				if (transmit_fnctn)
					send_escape("\033C",2);
				else
					scroller(CURSOR_RIGHT);
			}
			return;
		case K_HOME_ARROW:	/* HOME UP / DOWN */
			if (shift) {	/* HOME DOWN */
				if (transmit_fnctn)
					send_escape("\033F",2);
				else
					scroller(HOME_DOWN);
			} else {	/* HOME UP */
				if (transmit_fnctn)
					send_escape("\033h",2);
				else
					scroller(HOME_UP);
			}
			return;
		case K_INSERT_LINE:	/* insert line */
			if (transmit_fnctn)
				send_escape("\033L",2);
			else {
				scroller(INSERT_LINE, ite->ypos);
				scroller(SET_CURSOR, 0, ite->ypos);
			}
			return;
		case K_DELETE_LINE:	/* delete line */
			if (transmit_fnctn)
				send_escape("\033M",2);
			else {
				scroller(DELETE_LINE, ite->ypos);
				scroller(SET_CURSOR, 0, ite->ypos);
			}
			return;
		case K_PREV:	/* PREV PAGE */
		case K_RECALL:	/* recall -- PREV PAGE */
			if ((key==K_PREV) || (!shift)) {
				if (transmit_fnctn)
					send_escape("\033V",2);
				else
					scroller(PREV_PAGE);
				return;
			}
		case K_NEXT:	/* NEXT PAGE */
				/* shifted recall -- NEXT PAGE */
			if (transmit_fnctn)
				send_escape("\033U",2);
			else
				scroller(NEXT_PAGE);
			return;
		case K_INSERT_CHAR:	/* insert character */
			if (control && k->type!=K_NIMITZ) {
				/* Toggle ALPHA On/Off for ITF */
				if (ite->flags&ALPHAON)
					alpha_off();
				else
					alpha_on();
				return;
			}
			if (transmit_fnctn) {
				if (ite->insert_mode == TRUE) {
					send_escape("\033R",2);
				} else {
					send_escape("\033Q",2);
				}
			} else
				scroller(INSERT_MODE, ite->insert_mode^TRUE);
			return;
		case K_DELETE_CHAR:	/* delete character */
			if (control && k->type!=K_NIMITZ) {
				/* Toggle GRAPHICS On/Off for ITF */
				if (ite->flags&GRAPHICSON)
					graphics_off();
				else
					graphics_on();
				return;
			}
			if (transmit_fnctn)
				send_escape("\033P",2);
			else
				scroller(DELETE);
			return;
		case K_BACKSPACE:	/* back space */
			/* shifted back space on NIMITZ only - DEL*/
			if (shift && k->type==K_NIMITZ)
				key = DEL;
			else
				key = BS;
			break;
		case K_RUN:	/* run key - MODES */
			scroller(DSP_LABELS, MODES, 0);
			return;
		case K_EDIT:	/* edit */
			if (!shift) {
				kbd_beep();
				return;
			}
			/* shift edit == display functions */
		case K_STEP:	/* step -- Display Functions */
			/* Shift-step: any char key */
			if (shift && key==K_STEP) {
				return;
			}
			if (ite->xmit_fnctn && ite->disp_funcs==FALSE) 
				send_escape("\033Y",2);
			else
			        scroller(DISP_FUNCS, ite->disp_funcs^TRUE);
			return;
		case K_ALPHA:	/* alpha toggle */
			if (shift) { /* dump alpha */
				return;
			}
			if (ite->flags&ALPHAON)
				alpha_off();
			else
				alpha_on();
			return;
		case K_GRAPHICS: /* graphics toggle */
			if (shift) { /* dump graphics */
				return;
			}
			if (ite->flags&GRAPHICSON)
				graphics_off();
			else
				graphics_on();
			return;
		case K_CLR_DISP:	/* clear screen, ITF only*/
			if (transmit_fnctn)
				send_escape("\033J",2);
			else {
				if (control) { /* Control means WHOLE thing */
					scroller(HOME_UP);
					scroller(CLEAR_SCREEN);
				} else
					scroller(CLEAR_SCREEN);
			}
			return;
		case K_CLR_LINE:
		case K_CLEAR_TO_END:	/* clear to end - clear line */
		case K_N_CLEAR_LINE:	/* clear line */
			if (shift && key==K_N_CLEAR_LINE) { /* clear screen */
				if (transmit_fnctn)
					send_escape("\033J",2);
				else
					scroller(CLEAR_SCREEN);
				return;
			}
			/* clear to end of line */
			if (transmit_fnctn)
				send_escape("\033K",2);
			else
				scroller(CLEAR_LINE);
			return;
		case K_RESULT:	/* result */
			if (shift) /* set tab (if shifted) */
				scroller(SET_TAB);
			else
				kbd_beep();
			return;
		case K_PRT_ALL:	/* print all */
			if (shift) { /* clear tab (if shifted) */
				if (control)
					scroller(CLEAR_ALL_TABS);
				else
					scroller(CLEAR_TAB);
			} else
				kbd_beep();
			return;
		case K_CLR_IO:
			if (shift) /* stop - SOFT RESET */
				scroller(SCR_SOFT_RESET);
			else /* clr io -- Break */
				kbd_mute_char(FRERROR);
			return;
		case K_STOP:	/* Stop/Start Output to Screen */
			if (tp->t_state&TTSTOP)
				ite_proc(tp, T_RESUME, 0);
			else
				ite_proc(tp, T_SUSPEND, 0);
			return;
		case K_BREAK:	/* ITF150 Break key */
			if (shift) {	/* Local Resets */
				if (control) /* Hard Reset */
					scroller(SCR_HARD_RESET);
				else /* Soft Reset */
					scroller(SCR_SOFT_RESET);
			} else /* Interrupt local system, but sorry no
				  resuming allowed. */
				kbd_mute_char(FRERROR);
			return;
		case K_PAUSE:	/* pause - ESCAPE */
			/* reset - ESCAPE */ /* Control - HARD RESET */
			if (control) {
				scroller(SCR_HARD_RESET);
				return;
			} else
				if (k->type == K_NIMITZ)
					key = ESCAPE;
				else {
					scroller(SCR_SOFT_RESET);
					return;
				}
			break;
		case K_RETURN:	/* enter */
			kbd_mute_char((unsigned int)ite->retdef[0]);
			if (ite->retdef[1] != ' ') {
				kbd_mute_char((unsigned int)ite->retdef[1]);
			}
			return;
		case K_CONTINUE:
			if (shift) /* shifted continue -- user function define */
				scroller(DSP_LABELS, SET_USER, 0);
			else /* continue -- user function enable */
				scroller(DSP_LABELS, USER, 0);
			return;

		case K_ENTER:
		case (K_NP_ENTER&(~(K_NPAD<<8))):
		case K_EXECUTE:			/* execute - nimitz ENTER key */
			send_current_line(ite);
			return;

		case (K_NP_K0&(~(K_NPAD<<8))):
		case (K_NP_K1&(~(K_NPAD<<8))):
		case (K_NP_K2&(~(K_NPAD<<8))):
		case (K_NP_K3&(~(K_NPAD<<8))):
			/* Send LF if CR is send and autolf is true. */
		default:	/* unrecognized */
			kbd_beep();
			return;
		}
	}

	if (ite->xpos == 71) kbd_beep();	/* The 2622 does this! */
	kbd_mute_char(key&0x0FF);
}

extern unsigned char *user_key[];
extern char user_key_type[];

softkey_input(ite, parm1)
struct iterminal *ite;
{
	register struct tty *tp = ite->ite_tty;
	register unsigned c;
	register unsigned char *ptr;

	if (ite->key_state==USER || ite->key_state==CLEAR) {
		ptr = user_key[parm1-1];
		switch (user_key_type[parm1-1]) {
		case 'L':
			while (c = *ptr++) {
				crtio(c);
				if (ite->autolf_mode && c == CR)
					crtio(LF);
			}
			break;
		case 'N':
			send_escape(user_key[parm1-1],80);
			break;
		default:
		case 'T':
			if ((tp->t_state & (ISOPEN|WOPEN))==0)
				break;
			if (!ite->remote_mode)
				break;

			ite_flush_check(81);
			while (c = *ptr++) {
				(*linesw[tp->t_line].l_input)(tp, c, 0, tp->t_in_count);
				if (ite->autolf_mode && c==CR)
					(*linesw[tp->t_line].l_input)(tp, LF, 0, tp->t_in_count);
			}
			(*linesw[tp->t_line].l_input)(tp, CR, 0, tp->t_in_count);
			if (ite->autolf_mode)
				(*linesw[tp->t_line].l_input)(tp, LF, 0, tp->t_in_count);
		}
	}
	else
		scroller(SFK_INPUT, parm1);
}


/*
 * Send the current line as if it was typed in.
 */
send_current_line(ite)
register struct iterminal *ite;
{
	register int x, y, c, current_color, current_enhance;
	register SCROLL *buf, *line;
	SCROLL s;

	scroller(SET_CURSOR, 0, ite->ypos);	/* start of line for echoing */
	current_color=0;
	current_enhance=0;
	
	line = &ite->scroll_buf[((ite->ypos + ite->start_line) % ite->scroll_lines)*ite->screenwidth];
	for (x=0; x<ite->screenwidth; x++) {
		s = line[x];
		if (!char_on(s))
			break;

		/* Colors changed? */
		if (char_colorpair(s) != current_color) {
			current_color = char_colorpair(s);
			send_escape("\033&v", 3);
			kbd_mute_char(current_color+'0');
			kbd_mute_char('S');
		}

		/* Enhancements (inverse, underline) changed? */
		if ((s & CHAR_ENHANCE) != current_enhance) {
			current_enhance = s & CHAR_ENHANCE;
			c = '@';
			if (s & HALF_BRIGHT)	c += 8;
			if (s & UNDERLINE)	c += 4;
			if (s & INVERSE_VIDEO)	c += 2;
			if (s & BLINKING)	c += 1;
			send_escape("\033&d", 3);
			kbd_mute_char(c);
		}
		kbd_mute_char(s & CHAR_VAL);
	}
	kbd_mute_char((unsigned int)CR);
}


term_reset(type)
register short type;
{
	extern int kbd_reset();
	register struct iterminal *ite = &iterminal;
	register struct k_keystate *k = &k_keystate;
	register short i;

	kbd_beep();
	
	if (type == HARD) {
		/* reset terminal configuration to defaults */
		ite->remote_mode = TRUE;
		ite->local_echo = FALSE;
		ite->xmit_fnctn = FALSE;
		ite->insert_mode = FALSE;
		ite->disp_funcs = FALSE;
		ite->inheolwrp = FALSE;
		ite->escape = FALSE;

		kbd_reset();
		char_switch();
		if ((ite->key_state == TCONFIG) || (ite->key_state == SET_USER))
			scr_restore_dsp(ite, 0, 0);

		for (i = 0; i < NUMBER_OF_SFKS; i++)
			init_softkey(i, user_defaults[i][0], user_defaults[i][1]);

		scroller_init(HARD);
		scroller(REMOTE, TRUE);
		scroller(DSP_LABELS, CLEAR, 0);
		scroller(DSP_LABELS, MODES, 0);
		scroller(CLEAR_ALL_TABS);
		/* set users keys to defaults */

		/* turn on alpha display */
		scroller(ALPHA_ON);
	} else
		scroller_init(SOFT);

	scroller(DISP_FUNCS, FALSE);

}


send_escape(sequence,length)
unsigned char *sequence;
{
	register struct iterminal *ite = &iterminal;
	register struct tty *tp = ite->ite_tty;
	register unsigned int c;

	/* If sequence is going to overflow the input buffer then
	** flush it now
	*/
	if ((tp->t_state&(ISOPEN|WOPEN)))
	if ((ite->remote_mode) && (ite->key_state != TCONFIG) &&
		(ite->key_state != SET_USER) ) {
		ite_flush_check(length);
	}

	while (c = *sequence++)
		kbd_mute_char(c);
}


ite_out_char(c)
register unsigned c;
{
	register struct iterminal *ite = &iterminal;
	register struct tty *tp = ite->ite_tty;
	register int remote_mode;

	/*
	 * The state of ite->remote_mode might change via this first call
	 * to crtio, so we save the flag to treat the character consistently.
	 *
	 * This hapens with the last 'R' of <esc>&k1R, which turns on remote
	 * mode.  The 'R' is echoed because we're in local mode, and then
	 * it's sent to the line discipline because we're *now* in remote mode.
	 *
	 * I don't think that any of the other information, like 
	 * ite->key_state, can change via a call to crtio().
	 */
	
	remote_mode = ite->remote_mode;			/* save old state */
	if (ite->local_echo || !remote_mode ||
		ite->key_state == TCONFIG || ite->key_state == SET_USER) {
		crtio(c);
		if (ite->autolf_mode && c == CR)
			crtio(LF);
	}

	if ((tp->t_state&(ISOPEN|WOPEN)) && 
	    remote_mode && (ite->key_state != TCONFIG) &&
		(ite->key_state != SET_USER) ) {
		(*linesw[tp->t_line].l_input)(tp, c, 0, tp->t_in_count);
		if (ite->autolf_mode && c == CR)
			(*linesw[tp->t_line].l_input)(tp, LF, 0, tp->t_in_count);
	}

	/*
	 * Writing the character could have overwritten
	 * the cursor, so we restore it here if needed.
	 * 
	 * This could happen if we're in non-remote mode, or in one
	 * of the configuration screens.
	 */
	if (!ite->cursor_on)
		scroller(SET_CURSOR, ite->xpos, ite->ypos);
}


ite_self_id()
{
	register struct iterminal *ite = &iterminal;
	register struct tty *tp = ite->ite_tty;
	register char *p;


	if ((ite->remote_mode) && (ite->key_state != TCONFIG) &&
		(ite->key_state != SET_USER) ) {
		/* If sequence is going to overflow the input buffer
		** then flush it now.
		*/
		ite_flush_check(UTSLEN+1);
		p = (char *)&utsname.machine[0];
		while (*p)
			(*linesw[tp->t_line].l_input)(tp, (unsigned int)*p++, 0, tp->t_in_count);
		(*linesw[tp->t_line].l_input)(tp, CR, 0, tp->t_in_count);
		if (ite->autolf_mode)
			(*linesw[tp->t_line].l_input)(tp, LF, 0, tp->t_in_count);
	}
}

ite_flush_check(val)
{
	struct iterminal *ite = &iterminal;
	register struct tty *tp = ite->ite_tty;

	if (tp->t_rawq.c_cc+val > TTYHOG+2)
		ttyflush(tp, FREAD);
}

crtio(c)
unsigned c;
{
	register struct iterminal *ite = &iterminal;

	/* Handle display functions here. */
	if (ite->dsp_funcs)
		/* Handle display functions on config screen */
		conf_alpha(c);
	else if (ite->disp_funcs) {
		/* Handle display functions on scroller screen */
		scr_char_out(c);
		/* Handle weird stuff */
		switch (c) {
		case CR:
			scroller(SET_CURSOR, 0, ite->ypos);
			scroller(LF_CURSOR_DOWN);
			ite->escape = FALSE;
			break;
		case ESCAPE:
			ite->escape = TRUE;
				break;
		case 'Z':
			if (ite->escape)
				scr_disp_funcs(FALSE);
		default:
			ite->escape = FALSE;
			break;
		}
	} else {
		ite_pars(c); /* Parse ready character */
	}
}
