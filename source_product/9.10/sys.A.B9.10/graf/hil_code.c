/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/hil_code.c,v $
 * $Revision: 1.5.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 15:14:46 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988  Hewlett-Packard Company.
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
#include "../h/time.h"
#include "../h/kernel.h"
#include "../machine/cpu.h"
#include "../wsio/intrpt.h"
#include "../graf/kbd.h"
#include "../graf/hil.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../s200io/bootrom.h"

/* Array of pointers to existing HILs */

struct hilrec *hils[H_MAXHILS];

#ifndef	TRUE
#define	TRUE		1
#define	FALSE		0
#endif

#define KEYSHIFT	0x10	/* key was shifted  */
#define KEYCNTL		0x20	/* key was control key */

/* 8042 status register masks */ 
#define DATA_RDY	0x01
#define CMD_PEND	0x02

#undef timeout			/* Use timeout instead of Ktimeout */

int hil_timeout_period=1*HZ;	/* one second timeout (value is in ticks) */

hil_timeout(var)
int *var;
{
	*var = TRUE;
}

#define start_hil_loop							\
{									\
	int hil_timed_out;						\
	hil_timed_out = FALSE;						\
	timeout(hil_timeout, &hil_timed_out, hil_timeout_period);

#define end_hil_loop(string)						\
	untimeout(hil_timeout, &hil_timed_out);				\
	if (hil_timed_out)						\
		msg_printf("hil timeout: %s\n", (string));		\
}

int hil_count=0;


/*
 * Initialize a Human Interface Loop/Link.
 * Called once for each loop that we find.
 *
 * Returns number of devices found on the loop
 * or -1 if we reject this loop.
 */
hil_loop_init(addr, sc)
char *addr;
int sc;				/* select code */
{
	register struct hilrec *g;
	register struct devicerec *d;
	register struct hilbuf *gb;
	register int index;
	unsigned char data;
	extern int hil_isr();

	/* Allocate storage for the data structures */
	if (hil_count>=H_MAXHILS) {
		printf("Too many HIL cards: ");
		return -1;
	}

	g = hils[hil_count++] = (struct hilrec *) calloc(sizeof(struct hilrec));
	if (g==NULL)
		panic("hil_loop_init: can't allocate hilrec space");

	/* Initialize the data structures */
	g->hilbase = (struct data8042 *) addr;
	g->sc = sc;			/* 0 for internal card */
	g->command_device = g->active_device = g->packet_length = 0;
	g->command_ending = g->has_ite_kbd = g->hilkbd_took_ite_kbd = FALSE;
	g->collecting = FALSE;
	g->loopcmddone = TRUE;
	g->has_8042_data = FALSE;
	g->cooked_mask = g->current_mask = 0;
	g->num_ldevices = g->repeat_rate = 0;
	for (index=1; index<=H_LMAXDEVICES; index++) {
		d = &g->loopdevices[index];
		d->open_flag = FALSE;
		d->sleep = FALSE;
	}

	/* Set up an input queue for this HIl */
	gb = &g->gb;
	gb->g_in_head = gb->g_in_tail = gb->g_inbuf =
		(ushort *) calloc(HILBUF_SIZE*2);
	if (gb->g_in_head==NULL)
		panic("hil_loop_init: can't allocate HILBUF space");
	gb->g_in_count = 0;
	gb->g_int_flag = 0;
	gb->g_rcv_flag = FALSE;
	gb->g_int_lvl = 1;

	/* setup 8042 isr */
  	isrlink(hil_isr, 1, &g->hilbase->cmd_stat, DATA_RDY, DATA_RDY, 0, g);

	if (sc != 0)				/* not a built-in card */
		g->hilbase->interrupt = 0x88;	/* enable levels 1 & 7 intrpt */

	/* Do we have an HIL interface here? */
	hil_cmd(g, H_READCONFIG, NULL, 0, &data); /* get configuration */
	if (! (data & 0x20)) {			/* do we have an interface? */
		printf("No interface present: ");
		hil_count--;			/* we don't have this one */
		return -1;			/* no, bag it. */
	}

	/*
	 * Bit 7: force a loop reconfiguration.
	 * Bit 1: don't report loop errors.
	 * Bit 0: Enable auto-polling.
	 */
	data = 0x93;
	hil_command(g, H_WRITECTRL, &data, 1);
/***	NOTE: we could poll & wait instead of just waiting, e.g.:	***/
/***	for (index=0; index<1000; index++) {				***/
/***		if (g->hilbase->cmd_stat & DATA_RDY)			***/
/***			break;						***/
/***		snooze(1000);						***/
/***	}								***/
	snooze(1000000);  /* Wait 1 sec for the reconfiguration to finish */

	/* If the loop reconfigured, read the data register and toss it. */
	if (g->hilbase->cmd_stat & DATA_RDY)
		data = g->hilbase->hil_data;

	/* Now read the loop status register. */
	do {
		hil_cmd(g, H_READLSTAT, NULL, 0, &data);
	/* NOTE: what about timeout? */
	} while ((data & 0x88) == 0);

	/* Skip the rest if the loop did not reconfigure. */
	if (data & 0x08) {		/* If we successfully reconfigured: */
		g->num_ldevices = data & 0x07;
		/* Read the cooked keyboard mask. */
		hil_cmd(g, H_READKBDS, NULL, 0, &data);
		g->cooked_mask = g->current_mask = data;
	}
	hil_cmd(g, H_INTON, NULL, 0, NULL);	/* enable interrupts? */
	return g->num_ldevices;
}

/*
 * Here's how loop reconfiguration works:
 *
 * When you plug in a device, or unplug one, the loop reconfigures.
 * That is, the 8042 goes out and looks to see how many devices are out there
 * and which ones are keyboards, and then it tells us about what it found.
 *
 * This is accomplished in two phases.  First, an interrupt is sent to us
 * saying that loop reconfiguration begins.  Then, the 8042 resets & scans
 * the devices.  Then, an interrupt is sent saying that reconfiguration
 * is complete.  Problem is, if there are no devices on the loop, the
 * reconfiguration never ends!  Gack!
 *
 * We set a two-second timeout when reconfiguration begins.  If we don't
 * get a "reconfiguration complete" interrupt before the timeout, we assume
 * that the reconfiguration is hung and we say that we have 0 devices.
 */

reconfig_timeout(g)
register struct hilrec *g;
{
	msg_printf("reconfig_timeout: hil at sc %d has timed out\n", g->sc);
	g->num_ldevices = 0;		/* no devices */
}


/*-----------------------*/
/* Reconfigure the loop. */
/*-----------------------*/

hil_reconfigure(g)
register struct hilrec *g;
{
	unsigned char data, tmp_mask, index;

	/* Request the LPSTAT register from the 8042. */
	hil_cmd(g, H_READLSTAT, NULL, 0, &data);
	/* Save the number of devices on the loop. */
	g->num_ldevices = index = data & 0x07;
	/* Set up a new cooked keyboard mask. */
	g->cooked_mask = g->current_mask = 0;

	/* Read the cooked keyboard mask. */
	hil_cmd(g, H_READKBDS, NULL, 0, &data);
	g->cooked_mask = tmp_mask = data;

	/* If the device is opened and cooked, set it to raw mode. */
	for (; index; index--) {
	    if ((tmp_mask & (1<<(index-1)))	/* if cooked */
	    && g->loopdevices[index].open_flag)	/* and open */
			tmp_mask ^= 1<<(index-1);	/* uncook it */
	}

	/* Set the current cooked keyboard mask. */
	g->current_mask = tmp_mask;
	/* Update the cooked keyboard register on the 8042. */
	hil_command(g, H_WRITEKBDS, &tmp_mask, 1);
}

/*
 * 8042 input interrupt service routine.
 *
 * NOTE: possible improvement: loop here if the status indicates
 * pending interrupt (bit0?).  This will save lots of interrupt
 * servicing overhead.
 *
 * COUNTER-NOTE: that might have been a good idea on a 320, but modern
 * cpus are so much faster than HIL that there's never one pending yet
 * when we finish servicing the previous one.  Believe me, I tried it.
 */
hil_isr(int_data)
register struct interrupt *int_data;
{
	register struct hilrec *g = (struct hilrec *) int_data->temp;
	register unsigned char st, dt;

	/* Fetch data and status */
	st = g->hilbase->cmd_stat;
	dt = g->hilbase->hil_data;
	hil_queue(g, st, dt);
}

/*
 * keyboard input isr PART II
 *
 * This can be called from hil_isr() for normal HIL interrupts,
 * or from hil_nmi_isr() for the level-7 non-maskable interrupt.
 */
hil_queue(g, st, dt)
register struct hilrec *g;
unsigned char st, dt;
{
	register struct hilbuf *gb;
	register unsigned status, data;
	extern int hil_service();
	union {
		short	data;
		unsigned char	byte[2];
	} c;

	gb = &g->gb;

	/* Fetch data and status */
	c.byte[0] = st;
	c.byte[1] = dt;

	/* Don't enter this routine from NMI if we are already in it */
	if (gb->g_int_flag&NMI_LCK) {
		if (dt!=56) return;	/* Throw away if not PAUSE Key */
		if (!(st&KEYCNTL))
			gb->g_int_flag |= NMI_CNTL;
		else
			gb->g_int_flag |= NMI_SHFT;
		return;
	}

	/* Lock out service of keyboard nmi's for now */
	gb->g_int_flag |= NMI_LCK;
	/* Save data and status in the queue */
	/* Extra data is lost when buffer overflows */
nmi_srv:
	status = c.byte[0]>>4;
	data = c.byte[1];
	if (status==5) {
		if (!(data&H_LCOMMAND))
			if ((data&H_POLLING)&&
			    (gb->g_in_count+20 >= HILBUF_SIZE))
				g->collecting = FALSE;
			else
				g->collecting = TRUE;
	} else {
		if (status != 6) g->collecting = TRUE;
	}
	if (g->collecting) {
		if (gb->g_in_count < HILBUF_SIZE) {
			gb->g_in_count++;
			*gb->g_in_tail++ = c.data;
			if (gb->g_in_tail >= gb->g_inbuf+HILBUF_SIZE)
				gb->g_in_tail = gb->g_inbuf;
		} else {
			gb->g_in_count = gb->g_in_count;
		}
	}
	/* Signal level zero service to be done by ite_service */
	if (!gb->g_rcv_flag) {
		gb->g_rcv_flag = TRUE;
		sw_trigger(&gb->g_rcv_intloc, hil_service, g, 0, gb->g_int_lvl);
	}
	
	/* Check for pending NMI service */
	if (gb->g_int_flag&(NMI_CNTL|NMI_SHFT)) {
		if (gb->g_int_flag&NMI_CNTL)
			c.data = 0x8000+56;
		else
			c.data = 0xA000+56;
		gb->g_int_flag &= ~(NMI_CNTL|NMI_SHFT);
		goto nmi_srv;
	}
	gb->g_int_flag &= ~NMI_LCK;
}

int hil_rpg_device = 0;		/* No rpg device by default */
int reconfig_timeout();		/* forward reference */

/*----------------------------*/
/* Process a data/status pair */
/*----------------------------*/
hil_proc(g, data, status)
register struct hilrec *g;
register unsigned char data, status;
{
	extern hil_buffer();
	extern struct timeval time;
	register unsigned char i;
	register x;
	union {
		long	longtime;
		unsigned char	chartime[4];
	} timestamp;

	switch(status>>4) {
	default: /* None of these should occur. */
		/* case 0: Not Used */
		/* case 1: 10 ms periodic interrupt. */
		/* case 2: Special timer interrupt. */
		/* case 3: Special timer and 10ms periodic interrupt. */
		/* case 7: Power-up reset & selftest was completed. */
			/* This should be handled if gator box hil is ever
			   supported.  Powerup reconfigures the loop, so
			   the cooked_mask must be reset and the ite may
			   need to be re-initialized. */ 
		break;

	case 4: /* Data Requested by 68K is ready. */
		if (g->has_8042_data)
			msg_printf("hil_proc: requested data overflow\n");
		g->has_8042_data = TRUE;
		g->raw_8042_data = data;
		break;

	case 5:	/* Data Buffer has Caravan Status Code. */
		/* If an error, ignore it. */
		if (data&H_LERROR) {
			switch (data) {
			case H_LDOWN:		/* Reconfigure has started */
				timeout(reconfig_timeout, g, 2*HZ);
				break;
			case H_TIMEOUT:
				msg_printf("hil_proc: hil has timed out\n");
				/* fall thru to reconfigure */
			case H_LRECONFIG:	/* Loop has reconfigured. */
				untimeout(reconfig_timeout, g);	
				x=spl7();
				hil_reconfigure(g);
				splx(x);
				g->loopcmddone = TRUE;
				break;
			case H_LDERROR:
				msg_printf("hil_proc: data error\n");
				g->loopcmddone = TRUE;
				break;
			default:
				msg_printf("hil_proc: strange data=%x\n", data);
			}
			break;
		}
		/* Check for end of data or command. */
		if (data&H_LCOMMAND) {
			if (data&H_POLLING) {	/* End of polled data */
				g->packet[0] = g->packet_length;
				if (g->active_device==hil_rpg_device
				&& g->packet_length>=8
				&& (g->packet[5] & 02)) {
					if (g->packet[6])
					    hil_proc(g, ~g->packet[6],15<<4);
					if (g->packet[7])
					    hil_proc(g, g->packet[7],14<<4);
				}
				else
					hil_buffer(g);
			} else {		/* End of loop command */
				g->command_ending = TRUE;
			}
			/* Clear the active device address and reset the
			   buffer */
			g->active_device = 0;
		} else {
		/* Check for polled data. */
		if (data&H_POLLING) {
			if (g->active_device != 0) {
			/* End of current data and start of new data
			   within the same poll record. */
				g->packet[0] = g->packet_length;
				hil_buffer(g);
			}
			/* Set the new active device. */
			g->active_device = data&H_DEVADR;
			g->packet_length = 5;
			x=spl7();
			timestamp.longtime = time.tv_sec * 100 +
					     time.tv_usec / 10000;
			splx(x);
			for (i=1; i<5; i++)
				g->packet[i] = timestamp.chartime[i-1];
		} else {			/* Start of command data */
			if (g->command_device == (data&H_DEVADR)) {
				g->active_device = 0;
				g->response_length = 0;
			}
		}
		}
		break;
	case 6:	/* Data Buffer has Caravan Data Byte. */
		/* If there is an loop device actively collecting data, then
		** add the character to the packet; otherwise, ignore it. */
		if (g->active_device != 0) {
			if (g->packet_length >= MAX_PACKET_LENGTH)
				panic("g->packet overflow");
			g->packet[g->packet_length++] = data;
		}
		else if (g->command_device != 0)
			if (g->command_ending) {
				g->loopcmddone = TRUE;
				g->command_ending = FALSE;
			} else {
				if (g->response_length >= MAX_PACKET_LENGTH)
					panic("g->loop_response overflow");
				g->loop_response[g->response_length++] = data;
			}
		break;
	case 8: /* Key - shift and control */
	case 9: /* Key - control only */
	case 10:/* Key - shift only */
	case 11:/* Key - no shift, no control */
		/* "cooked" (nimitz) key interrupt
		** place time stamp, status, and key
		** in the appropriate buffer. ( use device address 8 )
		*/
		if (g->has_ite_kbd)
		/* If this loop has the ite keyboard, map the key
		   and send it to the tty handling routines. */
			kbd_map_keys(data, status);
		else {
		/* Otherwise return the cooked key strokes to the user. */
			x=spl7();
			timestamp.longtime = time.tv_sec * 100 +
					     time.tv_usec / 10000;
			splx(x);
			for (i=0; i<4; i++)
				g->packet[i] = timestamp.chartime[i];
			g->packet[4] = status ;
			g->packet[5] = data ;
			g->packet_length = 6;
			g->active_device = 8;
			hil_buffer(g);
			g->active_device = 0;
		}
		break;
	case 12:/* RPG - shift and control */
	case 14:/* RPG - shift only */
		if (g->has_ite_kbd) {
			/* Convert knob to keycodes */
			/* negative => cursor down; positive => cursor up */
			/* Indicate as unshifted (negative true shift bit) */
			kbd_map_keys(data>=128 ? K_KNOB_DOWN : K_KNOB_UP,
				     status|KEYSHIFT);
			break;
		}
	case 13:/* RPG - control only */
	case 15:/* RPG - no shift, no control */
		if (g->has_ite_kbd) {
			/* Convert knob to keycodes */
			/* negative => cursor right; positive => left */
			/* Indicate as unshifted (negative true shift bit) */
			kbd_map_keys(data>=128 ? K_KNOB_RIGHT : K_KNOB_LEFT,
				     status|KEYSHIFT);
		} else {
		/* RPG interrupt - place the time
		** stamp, status, and RPG count
		** in the appropriate buffer. ( use device address 9 )
		*/
		x=spl7();
		timestamp.longtime = time.tv_sec * 100 +
				     time.tv_usec / 10000;
		splx(x);
		for (i=0; i<4; i++)
			g->packet[i] = timestamp.chartime[i];
		g->packet[4] = status ;
		g->packet[5] = data ;
		g->packet_length = 6;
		g->active_device = 9;
		hil_buffer(g);
		g->active_device = 0;
		}
		break;
	}
}

/*-----------------------------*/
/* Level Zero Service for 8042 */
/*-----------------------------*/
hil_service(g)
register struct hilrec *g;
{
	register struct hilbuf *gb;
	union {
		short	data;
		unsigned char	byte[2];
	} val;
	register x;

	gb = &g->gb;

	while (1) {
		/* Fetch Data and Status */
		x = spl7();	/* Level 7 because NMI is there */
		if (gb->g_in_count == 0) {
			/* Indicate that software trigger has been disarmed */
			gb->g_rcv_flag = FALSE;
			splx(x);
			return;
		}
		gb->g_in_count--;
		val.data = *gb->g_in_head++;
		if (gb->g_in_head >= gb->g_inbuf+HILBUF_SIZE)
			gb->g_in_head = gb->g_inbuf;
		splx(x);

		hil_proc(g, val.byte[1], val.byte[0]);
	}
}

/*--------------------------------------------------*/
/* Send a command to the 8042; response is optional */
/*--------------------------------------------------*/
hil_cmd(g, cmd, string, length, returned_data)
register struct hilrec *g;
unsigned char cmd;
unsigned char *string, *returned_data;
{
	/* Send the command to the 8042 */
	hil_command(g, cmd, string, length);

	if (returned_data != NULL)
		*returned_data = hil_wait(g);
}

/*----------------------------*/
/* Send a command to the 8042 */
/*----------------------------*/
hil_command(g, cmd, string, length)
register struct hilrec *g;
unsigned char 	cmd;
unsigned char	*string;
{
	hilrdywait(g);
	g->hilbase->cmd_stat = cmd;
	while (length-- != 0) {
		hilrdywait(g);
		g->hilbase->hil_data = *string++;
	}
}

/*---------------------------------*/
/* Wait until the 8042 is not busy */
/*---------------------------------*/
hilrdywait(g)
register struct hilrec *g;
{
	start_hil_loop;
	while ((g->hilbase->cmd_stat & CMD_PEND) && !hil_timed_out);
	end_hil_loop("hilrdywait");
}

/*------------------------------------------------------*/
/* Wait until the 8042 has returned the requested data. */
/*------------------------------------------------------*/
hildatawait(g)
register struct hilrec *g;
{
	start_hil_loop;
	while ((g->hilbase->cmd_stat & DATA_RDY) == 0 && !hil_timed_out);
	end_hil_loop("hildatawait");
}


/*------------------------------------------------------*/
/* Support for IOCTL is from this point to end of file. */
/*------------------------------------------------------*/

/*
 * Poll for data expected in response to a loop command
 *
 * If we're at interrupt level 0, then we don't have to poll.
 * Otherwise, we have to poll the hardware and call the service
 * routine ourselves.  We get called at high priority at
 * initialization time (hil_init).
 */
hil_pollisr(g)
register struct hilrec *g;
{
	register prio;
	register unsigned char st, dt;

	splx(prio=spl7());	/* find current interrupt level */

	if (prio>0) {		/* If HIL interrupts (level 1) are blocked: */
		/* Wait for data to appear */
		hildatawait(g);

		/* Fetch data and status and handle it now */
		st = g->hilbase->cmd_stat;	/* must fetch status first? */
		dt = g->hilbase->hil_data;
		hil_proc(g, dt, st);
	}
}


/*-----------------------------------------------*/
/* Send data and command to a device on the loop */
/*-----------------------------------------------*/
hil_ldcmd(g, loopdevice, loopcmd, loop_data, length)
register struct hilrec *g;
register int length;
unsigned char loopdevice, loopcmd, *loop_data;
{
	unsigned char data[3];

	/* Turn autopolling off */
	hil_polloff(g);

	/* Send the data to the loop */
	g->command_device = loopdevice;		/* must be after polloff */
	g->loopcmddone = FALSE;
	while (length-- != 0) {
		data[0] = loopdevice;		/* Device Address */
		data[1] = *loop_data++;		/* Data for a device */
		data[2] = H_TIMEOUT20;		/* 20 milliseconds */
		hil_command(g, H_STARTCMD, data, 3);
		hil_command(g, H_TRIGGER, NULL, 0);
		hilrdywait(g);
	}
	/* Send the command to the device on the loop */
	data[0] = 8 + loopdevice;	/* Command & Address */
	data[1] = loopcmd;		/* Command for a device */

	/*  The PST (Perform Self Test) request needs to wait a longer  */
	/*  period before timing out than the rest of the commands      */

	if (loopcmd == H_HIL_PST)
             data[2] = H_TIMEOUT200;    /* 200 milliseconds */
        else data[2] = H_TIMEOUT20;     /* 20 milliseconds */

	hil_command(g, H_STARTCMD, data, 3);
	hil_command(g, H_TRIGGER, NULL, 0);
	hilrdywait(g);

	/* If data is expected, then poll for it */
	/* Watch out for removing the device */
	do {
		hil_pollisr(g);
	} while (!g->loopcmddone && g->num_ldevices >= loopdevice);
	g->command_device = 0;		/* no command device now */

	/* Change to 8042 normal mode */
	hil_pollon(g);

	/* Reset auto repeat rate */
	data[0] = g->repeat_rate;
	hil_command(g, 0xA2, data, 1);
}


/*---------------------------------*/
/* Send Commands to device on loop */
/*---------------------------------*/
hil_lcommand(g, loopdevice, loopcmd)
register struct hilrec *g;
unsigned char loopdevice, loopcmd;
{
	hil_ldcmd(g, loopdevice, loopcmd, NULL, 0);
}


/*----------------------*/
/* Turn off autopolling */
/*----------------------*/
hil_polloff(g)
register struct hilrec *g;
{
	unsigned char data;
	unsigned char zero=0;

	/* Set auto repeat rate to zero */
	hil_command(g, 0xA2, &zero, 1);

	/* Turn autopolling off */
	hil_command(g, H_READCTRL, NULL, 0);
	data = hil_wait(g);

	data &= ~H_DOAUTOPOLL;
	hil_command(g, H_WRITECTRL, &data, 1);

	do {	/* Wait for Polling to Stop */
		hil_command(g, H_READBUSY, NULL, 0);
		data = hil_wait(g);
	} while (data&0x04);

	/* 
	 * Delay at least 20ms to allow for timing window on the
	 * faulty 8042 which will set up timeout to infinity
	 * if we do something too soon after stopping auto-polling.
	 */
	 snooze(25000);		/* Let's make it 25ms just to be sure. */
}


/*---------------------*/
/* Turn on autopolling */
/*---------------------*/
hil_pollon(g)
register struct hilrec *g;
{
	unsigned char data;

	/* Turn autopolling on */
	/* Change to 8042 normal mode */
	hil_command(g, H_READCTRL, NULL, 0);
	data = hil_wait(g);
	data |= H_DOAUTOPOLL;

	/* Start Auto Polling */
	hil_command(g, H_WRITECTRL, &data, 1);
}


hil_wait(g)
register struct hilrec *g;
{
	start_hil_loop;
	while (!g->has_8042_data && !hil_timed_out)
		hil_pollisr(g);
	g->has_8042_data = FALSE;
	end_hil_loop("hil_wait");

	return g->raw_8042_data;
}

extern int (*make_entry)();
int (*hil_saved_make_entry)();

hil_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	if (id!=0x29)
		return(*hil_saved_make_entry)(id, isc);

	isc->card_type = HIL_CARD;
	if (hil_loop_init(isc->card_ptr, isc->my_isc) == -1)
		return io_inform("HP91225A HIL interface", isc, -2, " rejected");
	return io_inform("HP91225A HIL interface", isc, 0);
}

hil_link()
{
	hil_saved_make_entry = make_entry;
	make_entry = hil_make_entry;
}


/*
 * Initialize the internal HIL card.
 * It's not at a regular select code, so the backplane scan
 * doesn't catch it.
 */
internal_hil_init()
{
	if (!(sysflags&NOKBD)) {	/* Do we have hil? */
		/* use pseudo-sc 0 */
		if (hil_loop_init((0x420000 + LOG_IO_OFFSET), 0) == -1)
		    printf("Internal HIL at 0x428000 rejected\n");
		else
		    printf("Internal HIL at 0x428000\n");
	}
}

/*
 * Check to see if one of the HIL keyboards generated an NMI.
 * Returns:
 * 	0: if there was no keyboard interrupt
 *	1: if it was an NMI and we handled it
 *     -1: if it was an shift-control-break and the monitor should be called
 */

extern int monitor_on;
#define	HPIBSTATUS (*((char *) (0x478005 + LOG_IO_OFFSET)))
hil_nmi_isr()
{
	int i;
	register struct hilrec *h;
	char modifiers;
	int status;

	for (i=0; i<hil_count; i++) {
		h = hils[i];
		if (h->sc==0) {			/* built-in HIL card */
			if (!(HPIBSTATUS & 0x04))
				continue;
		}
		else {				/* DIO HIL card */
			if (!(h->hilbase->interrupt & 0x04))
				continue;
		}

		/* Clear the NMI */
		/*
		 * Don't use hil_command or hil_cmd, because they
		 * set timeouts.  To do that, they fool around with the
		 * interrupt level, which can make us recurse before
		 * we clear the interrupt.
		 */
		while (h->hilbase->cmd_stat & CMD_PEND); /* wait for last cmd */
		h->hilbase->cmd_stat = H_CLEARNMI;	 /* Stop interrupting */

		/*
		 * Unfortunately, an NMI is generated for both shift-break
		 * and control-shift-break.  If it's just shift-break,
		 * (or if we have no monitor), then fake the appropriate key.
		 * That is, pretend that the user typed shift-break
		 * or control-shift-break.
		 */

		/* What are the modifiers? */
		while (h->hilbase->cmd_stat & CMD_PEND); /* wair for last cmd */
		h->hilbase->cmd_stat = H_GETMODIFIERS;   /* request modifiers */
		while ((h->hilbase->cmd_stat & DATA_RDY) == 0);
		status = h->hilbase->cmd_stat;
		modifiers = h->hilbase->hil_data;

		if (modifiers & 0x02)		/* was control pressed? */
			hil_queue(h, 0xA0, 56); /* no, fake shift-break */
		else {				/* control was pressed */
			if (monitor_on)		/* if we have a monitor */
				return -1;	/* go and do it */
			hil_queue(h,  0x80, 56);/* fake control-shift break */
		}
		return 1;			/* we handled it */
	}

	return 0;				/* wasn't us! */
}
