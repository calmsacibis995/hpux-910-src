/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/hil_drv.c,v $
 * $Revision: 1.6.84.4 $	$Author: drew $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/11/29 08:26:54 $
 */

/* HPUX_ID: @(#)hil_drv.c	55.1		88/12/23 */

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

/*
 8042 Drivers accessable as a user-land devices.
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/systm.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../graf/hil.h"
#include "../s200io/hilioctl.h"
#include "../wsio/beeper.h"
#include "../wsio/audio.h"

extern struct hilrec *hils[];
extern char ite_present;

#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif

#define HSEL_COLL	1	/* Select Collision Flag */

void
audio_beeper_hook(audio_des,frequency,attenuation,duration,beep_type)
    register struct audio_descriptor *audio_des;
    unsigned int frequency;
    unsigned int attenuation;
    unsigned int duration;
    int beep_type;
{
	return;
}

void (*audiobeeper)() = audio_beeper_hook;
struct audio_descriptor *audio_devices = (struct audio_descriptor *)0;

audio_beeper_time_hook(audio_des)
    register struct audio_descriptor *audio_des;
{
    return;
}
int (*audiobeepertime)() = audio_beeper_time_hook;

/*
 * Find a struct hilrec given a dev_t.
 *
 * The internal HIL loop has a select code of 0 in the table.
 */
struct hilrec *
which_hil(dev)
{
	register int i;
	register int select_code = (dev>>16) & 0xff;
	extern int hil_count;			/* how many loops we have */

	/* Search for this one in the list */
	for (i=0; i<hil_count; i++) {
		if (select_code==hils[i]->sc)
			return hils[i];
	}
	return NULL;				/* no such loop */
}

r8042_open(dev, flag)
{
	register struct hilrec *g;
	
	g = which_hil(dev);

	if (g == NULL)
		return(ENXIO);

	return(0);
}


r8042_close(dev)
{
	return(0);
}


/*
 * This routine is for HIL testing purposes.
 * The ioctl EFTSSD (Send Status & Data) takes two bytes of arguments,
 * HIL status & data bytes.  It software triggers to level 1, which is
 * where HIL interrupts.  When pkbd_do_isr() is invoked, it takes
 * the bytes and pretends that they arrived through an actual interrupt.
 *
 * In this manner, a programmer can send fake HIL packets, e.g.,
 * keystrokes or mouse buttons.
 */
pkbd_do_isr(status_data)
{
	int saveit;

	saveit = hils[0]->has_ite_kbd;	/* save old flag */
	hils[0]->has_ite_kbd = TRUE;	/* send this to the ITE! */

	hil_proc(hils[0], status_data & 0xff, status_data >> 8);

	hils[0]->has_ite_kbd = saveit;	/* restore old flag */
}

#ifndef EFTSSD
#define EFTSSD  _IOW('H',0x80,struct eft2) /* Fake status/data bytes */
#endif

r8042_ioctl(dev, cmd, arg, mode)
unsigned char *arg;
{
	register struct hilrec *g;
	register unsigned char index;
	register unsigned char jndex;
	register int ioc_size = (cmd&H_IOCSIZE)>>16;
	register unsigned char ioc_cmd = (cmd&H_IOCCMD);
	unsigned char cb[5], data;
	int x;
	static struct sw_intloc intloc;		/* zero fill for sw_trigger */
	struct beep_info *biptr;

	g = which_hil(dev);
	/* Execute the specified command */
	switch(cmd) {
	case EFTSSD:	/* Fake status & data from the user */
		if (!suser())			/* potential for evil stuff */
			return(EPERM);		/* trash non-super-user */
		x = spl6();
		sw_trigger(&intloc, pkbd_do_isr, (arg[0]<<8)+arg[1], 1, 0);
		splx(x);
		break;
	case EFTRIUR:	/* Read the in use register */
	case EFTRIM:	/* Read the interrupt mask */
	case EFTRKS:	/* Read the keyboard status register. */
	case EFTRCC:	/* Read the configuration code */
	case EFTRLC:	/* Read the language code */
	case EFTRKBD:	/* Read "cooked" keyboard mask */
	case EFTRLPS:	/* Read HIL interface status register */
	case EFTRLPC:	/* Read HIL interface control register */
	case EFTRREV:	/* Read the 8042 REVID */
		/* Read one byte of data from the 8042 */
		hil_cmd(g, ioc_cmd, NULL, 0, arg);
		break;
	case EFTSKBD:	/* Set the "cooked keyboard mask */
                /* Clear the autorepeat register. */
                data = 0;
		hil_cmd(g, 0x87, &data, 1, NULL);
		*arg &= g->cooked_mask;		/* Restrict to cookable stuff */
	case EFTSRD:	/* Set the repeat delay */
	case EFTSRR:	/* Set the repeat rate */
	case EFTSRPG:	/* Set the RPG repeat rate */
	case EFTSLPC:	/* Set HIL interface control register */
		/* Write one byte of data to the 8042 */
		/* Send the command and data to the 8042 */
		hil_cmd(g, ioc_cmd, arg, 1, NULL);
		if (cmd == EFTSRR) g->repeat_rate = arg[0];
		break;
	case EFTRRT:	/* Read the real time */
		/* First send a command to transfer the real time to the 8042
		   data buffer */
		hil_cmd(g, ioc_cmd, NULL, 0, NULL);
		/* Read each byte of the real time */
		for (index=0; index<=4; index++) {
			hil_cmd(g, H_READTIME+index, NULL, 0, &data);
			arg[4-index] = data;
		}
		break;
	case  EFTSIM:	/* Set the interrupt mask */
		/* Get the interrupt mask from the user buffer */
		data = arg[0]&H_INT_MASK;
		/* Send the command and data to the 8042 */
		hil_cmd(g, ioc_cmd|data, NULL, 0, NULL);
		break;
	case EFTSBI:	/* Set the bell information */

		if (audio_devices == (struct audio_descriptor *)0) {

		    /* Get the duration and frequency from the user buffer */
		    /* Send the command and data to the 8042 */

		    hil_cmd(g, ioc_cmd, arg, 2, NULL);
		}
		else {

		    /* Call audio driver beeper compatibility function */

		    (*audiobeeper)((struct audio_descriptor *)0,
				 arg[1] & 0xff,0,arg[0] & 0xff,
				 BEEPTYPE_200);
		}
		break;
	case EFTSRT:	/* Set the real time */
		/* Get the real time from the user buffer, lsb first */
		for (index=0; index<=4; index++)
			cb[index] = arg[4-index];
		/* Send the command and data to the 8042 */
		hil_cmd(g, ioc_cmd, cb, 5, NULL);
		break;
	case EFTSBP:	/* Send data to the beeper */
		if (audio_devices == (struct audio_descriptor *)0) {

		    /* Send four data bytes to the tone generator. */

		    hil_cmd(g, H_WRITEDATA, arg, 4, NULL);

		    /* Send the trigger beeper command to the 8042. */

		    hil_cmd(g, ioc_cmd, NULL, 0, NULL);
		}
		else {

		    /* Call audio driver beeper compatibility function */

		    /* Audio driver beeper only supports one voice */

		    (*audiobeeper)( (struct audio_descriptor *)0,
				  ((arg[1] << 4) | (arg[0] & 0x0f)),
				  (arg[2] & 0x0f),
				  arg[3] & 0xff,
				  BEEPTYPE_300
				);
		}
		break;
	case EFTRR0:	/* Read the general purpose data buffer */
	case EFTRT:	/* Read the timers for the four voices */
		if (audio_devices == (struct audio_descriptor *)0) {

		    /* Read each byte of the data */

		    for (index=0; index<=3; index++) {
			    hil_cmd(g,ioc_cmd+index,NULL,0,&data);
			    arg[index] = data;
		    }
		}
		else {

		    /* audio driver beeper only supports one voice */

		    arg[0] = (*audiobeepertime)((struct audio_descriptor *)0);
		    arg[1] = arg[2] = arg[0];
		    arg[3] = 0;
		}
		break;

	case DOBEEP:  /* General device independent beep ioctl */
		biptr = (struct beep_info *)arg;
		if (audio_devices == (struct audio_descriptor *)0) {

		    unsigned int frequency   = biptr->frequency;
		    unsigned int duration    = biptr->duration;
		    unsigned int attenuation = biptr->volume;

		    /* Convert general parameters to device specific */

		    if (frequency == 0)
			frequency = 1;

		    frequency = (83333 + (frequency / 2)) / frequency;
		    if (frequency > 1023)
			frequency = 1023;

		    if (attenuation > 100)
			attenuation = 100;

		    attenuation = (105 - attenuation) / 7;

		    if (duration == 0)
			attenuation = 15;
		    else {
			duration = (duration + 9) / 10;
			if (duration > 255)
			    duration = 255;
		    }

		    /* Pack values into EFTSBP format */

		    cb[0] = (0xc0 | (frequency & 0x0f));
		    cb[1] = frequency >> 4;
		    cb[2] = (0xd0 | attenuation);
		    cb[3] = duration;

		    /* Send four data bytes to the tone generator. */

		    hil_cmd(g, H_WRITEDATA, cb, 4, NULL);

		    /* Send the trigger beeper command to the 8042. */

		    hil_cmd(g, (EFTSBP & H_IOCCMD), NULL, 0, NULL);
		}
		else {

		    /* Call audio driver beeper function */

		    (*audiobeeper)( (struct audio_descriptor *)0,
				 biptr->frequency,biptr->volume,
				 biptr->duration,
				 BEEPTYPE_GENERAL);
		}
		break;


	default:
		/* Unexpected ioctl command */
		return(EINVAL);
		break;
	}
	return(0);
}


hil_open(dev, flag)
{
	register struct hilrec *g;
	register int loopdevice = ((dev&H_HIL_DEVICE)>>4);
	register struct devicerec *dv;
	register struct cblock *cp;
	unsigned char data, mask;
	
	g = which_hil(dev);

	if (g==NULL || loopdevice>H_LMAXDEVICES || loopdevice==0)
		return(ENXIO);
	if (loopdevice > g->num_ldevices)
		return(ENXIO);

	dv = &g->loopdevices[loopdevice];
	if (dv->open_flag)
		return(EBUSY);

	dv->open_flag = TRUE;
	/* Flush out this devices buffer */
	while ((cp = getcb(&dv->hdevqueue)) != NULL)
		putcf(cp);
	/* Enable HIL interrupts. */
	hil_cmd(g, H_INTON, NULL, 0, NULL);
	/* Put device in raw mode. */
	mask = 1 << (loopdevice - 1);
	if (mask & g->current_mask) {
		/* Clear the autorepeat register. */
		data = 0;
		hil_cmd(g, 0x87, &data, 1, NULL);
		g->current_mask ^= mask;
		/* Set the cooked keyboard mask. */
		hil_cmd(g, H_WRITEKBDS, &g->current_mask, 1, NULL);
	}

	return(0);
}


hil_close(dev)
{
	register struct hilrec *g;
	register int loopdevice = ((dev&H_HIL_DEVICE)>>4);
	register struct devicerec *dv;
	register unsigned char ioc_cmd = (HILDKR & H_IOCCMD);
	unsigned char mask;
	
	g = which_hil(dev);
	dv = &g->loopdevices[loopdevice];
	dv->open_flag = FALSE;

	/*
         * Disable autorepeat on device so keyboard driver
         * doesn't get confused
         */
        hil_ldcmd(g, (unsigned char)loopdevice, ioc_cmd, NULL, 0);

	/* Put device in cooked mode if necessary. */
	mask = 1 << (loopdevice - 1);
	if (mask & g->cooked_mask) {
		g->current_mask ^= mask;
		/* Set the cooked keyboard mask. */
		hil_cmd(g, H_WRITEKBDS, &g->current_mask, 1, NULL);
	}
	return(0);
}


hil_read(dev,uio)
register struct uio *uio;
{
	register x, y;
	register struct clist *tq;
	register struct hilrec *g;
	register struct hilbuf *gb;
	register struct devicerec *dv;
	register int loopdevice = ((dev&H_HIL_DEVICE)>>4);
	int err = 0;
	struct a {
		int fdes;
		};

	g = which_hil(dev);
	/* Set up the buffer and queue pointers */
	gb = &g->gb;
	dv = &g->loopdevices[loopdevice];
	tq = &dv->hdevqueue;
	if (tq->c_cc == 0) {
		x = splsx(gb->g_int_lvl);
		while (tq->c_cc == 0) {
			if (uio->uio_fpflags&FNDELAY)
				break;
			dv->sleep = TRUE;
			y = splx(gb->g_int_lvl);
			splsx(x);
			sleep((caddr_t)dv, HILIPRI);
			x = splsx(gb->g_int_lvl);
			splx(y);
		}
		splsx(x);
	}
	while (uio->uio_resid!=0 && err==0) {
		if (uio->uio_resid >= CBSIZE) {
			register n;
			register struct cblock *cp;

			cp = getcb(tq);
			if (cp == NULL)
				break;
			n = min(uio->uio_resid, cp->c_last - cp->c_first);
			err = uiomove(&cp->c_data[cp->c_first],n,UIO_READ, uio);
			putcf(cp);
		} else {
			register c;

			if ((c = getc(tq)) < 0)
				break;
			err = ureadc(c, uio);
		}
	}
	return(err);
}

hil_ioctl(dev, cmd, arg, mode)
register unsigned char *arg;
{
	register struct hilrec *g;
	register int loopdevice = ((dev&H_HIL_DEVICE)>>4);
	register int ioc_size = (cmd&H_IOCSIZE)>>16;
	register unsigned char ioc_cmd = (cmd&H_IOCCMD);
	register short index;
	unsigned char cb[3];

	g = which_hil(dev);
	if (loopdevice > g->num_ldevices)
		return(ENXIO);

	g->response_length = 0;
	/* Execute the specified command */
	switch(cmd) {
	case HILID:	/* Identify and Describe */
	case HILPST:	/* Perform Self Test */
	case HILRN:	/* Report Name */
	case HILRS:	/* Report Status */
	case HILED:	/* Extended Describe */
	case HILSC:	/* Report Security Code */
		/* Send the command to the HIL loop and wait for the
		   selected device to respond with data */
		hil_lcommand(g, loopdevice, ioc_cmd);
		for (index=0; index<g->response_length; index++)
			arg[index] = g->loop_response[index];
		break;
	case HILRR:	/* Read Register */
		/* The register number must be less than 128 */
		if (arg[0] > 127)
			return(EINVAL);

		/* Send the read register command to the loop device
		   and wait for the data */
		hil_ldcmd(g, loopdevice, ioc_cmd, arg, 1);
		arg[0] = g->loop_response[0];
		break;
	case HILDKR:	/* Disable Key Repeat */
	case HILER1:	/* Enable Key Repeat, 1/30 */
	case HILER2:	/* Enable Key Repeat, 1/60 */
	case HILP1:	/* Prompt 1 */
	case HILP2:	/* Prompt 2 */
	case HILP3:	/* Prompt 3 */
	case HILP4:	/* Prompt 4 */
	case HILP5:	/* Prompt 5 */
	case HILP6:	/* Prompt 6 */
	case HILP7:	/* Prompt 7 */
	case HILP:	/* Prompt */
	case HILA1:	/* Acknowledge 1 */
	case HILA2:	/* Acknowledge 2 */
	case HILA3:	/* Acknowledge 3 */
	case HILA4:	/* Acknowledge 4 */
	case HILA5:	/* Acknowledge 5 */
	case HILA6:	/* Acknowledge 6 */
	case HILA7:	/* Acknowledge 7 */
	case HILA:	/* Acknowledge */
		/* Send the command to the HIL loop */
		hil_lcommand(g, loopdevice, ioc_cmd);
		break;
	default:
		/* Check for write register command. */
		if ((cmd&0xc000ffff) == (IOC_IN|'h'<<8|0x07)) {
			/* Write Register */
			hil_ldcmd(g, loopdevice, ioc_cmd, arg, ioc_size);
			break;
		} else if ((cmd&0x0000ff00) == ('h'<<8)
		       &&  (ioc_cmd >= 0x80 && ioc_cmd <= 0xEF)) {
		/* 
		 * Device Specific command 
		 * Send the command and any data to the HIL loop
		 * and wait for the selected device to respond with
		 * data 
		 */
			hil_lcommand(g, loopdevice, ioc_cmd);
			/*
			 * If there is return data to be sent back
			 * to the user we must copy it into the
			 * data structure they gave us.
			 * If the size of the data structure that
			 * the user has given us is less than or
			 * equal to the response
			 * length then we must return an error.
			 * If the data struct. is > to the
			 * response length then give it all to them.
			 * Put the length plus the 1 into the first
			 * byte of the returned data.
			 */
			if (ioc_size > 0)
			    arg[0] = g->response_length+1;
			if (g->response_length > 0) {
			    if (ioc_size > g->response_length) {
				bcopy(g->loop_response,&arg[1],
				      g->response_length);
			     }
			     else
				return(EINVAL);
			}
			break;
		}

		/* else Unexpected ioctl command */
		return(EINVAL);
	}
	/* Did they swipe the device from us after we started?? */
	if (loopdevice > g->num_ldevices)
		return ENXIO;

	return(0);
}


hil_buffer(g)
register struct hilrec *g;
{
	register struct devicerec *dv;
	register struct clist *tq;
	register i;

	dv = &g->loopdevices[g->active_device];
	tq = &dv->hdevqueue;

	/* If no room for the codes, throw them away */
	if (tq->c_cc <= (HILHOG-g->packet_length)) {

		for (i=0; i<g->packet_length; i++)
			putc(g->packet[i], tq);
	}
	if (dv->sleep) {
		dv->sleep = FALSE;
		wakeup((caddr_t)dv);
	}
	/* Check for selects to wakeup */
	hilselwakeup(&dv->hil_sel);
	return(0);
}


hil_select(dev, which)
{
	register struct hilrec *g;
	register int loopdevice = ((dev&H_HIL_DEVICE)>>4);
	register struct devicerec *dv;
	
	g = which_hil(dev);

	dv = &g->loopdevices[loopdevice];
	switch(which) {
	case FREAD:
		if (dv->hdevqueue.c_cc)
			return(1);
		hilselqueue(&dv->hil_sel);
		break;
	}
	return(0);
}


hilselqueue(hilsel)
struct hilselect *hilsel;
{
	register struct proc *p;

	if ((p = hilsel->hil_selp) && (p->p_wchan == (caddr_t) &selwait))
		hilsel->hil_selflag |= HSEL_COLL;
	else
		hilsel->hil_selp = u.u_procp;
	return(0);
}


hilselwakeup(hilsel)
struct hilselect *hilsel;
{
	if (hilsel->hil_selp) {
		selwakeup(hilsel->hil_selp, hilsel->hil_selflag & HSEL_COLL);
		hilsel->hil_selp = 0;
		hilsel->hil_selflag &= ~HSEL_COLL;
	}
	return(0);
}


nimitz_open(dev, flag)
{
	register struct hilrec *g;
	register int loopdevice = ((dev&H_HIL_DEVICE)>>4);
	register struct devicerec *dv;
	register struct cblock *cp;
	
	g = which_hil(dev);

	if (g==NULL || loopdevice<H_NIMITZKBD || loopdevice>H_NIMITZKNOB)
		return(ENXIO);
	if (g->num_ldevices <= 0)
		return(ENXIO);

	dv = &g->loopdevices[loopdevice];
	if (dv->open_flag)
		return(EBUSY);

	dv->open_flag = TRUE;
	/* flush out htis devices buffer */
	while ((cp = getcb(&dv->hdevqueue)) != NULL)
		putcf(cp);
	if (g->has_ite_kbd) {
	/* Disable the keyboard from the ite. */
		g->has_ite_kbd = FALSE;
		g->hilkbd_took_ite_kbd = TRUE;
	}

	return(0);
}


nimitz_close(dev)
{
	register struct hilrec *g;
	register int loopdevice = ((dev&H_HIL_DEVICE)>>4);
	register struct devicerec *dv;
	register struct cblock *cp;
	
	g = which_hil(dev);
	dv = &g->loopdevices[loopdevice];
	dv->open_flag = FALSE;

	if (g->hilkbd_took_ite_kbd) {
		/* Flush the buffer for this device */
		while ((cp = getcb(&dv->hdevqueue)) != NULL)
			putcf(cp);
		/* Re-enable the ite keyboard. */
		g->hilkbd_took_ite_kbd = FALSE;
		g->has_ite_kbd = TRUE;
	}
}


nimitz_read(dev,uio)
register struct uio *uio;
{
	register x, y;
	register struct clist *tq;
	register struct hilrec *g;
	register struct hilbuf *gb;
	register struct devicerec *dv;
	register int loopdevice = ((dev&H_HIL_DEVICE)>>4);
	int err = 0;
	struct a {
		int fdes;
		};

	g = which_hil(dev);
	/* Set up the buffer and queue pointers */
	gb = &g->gb;
	dv = &g->loopdevices[loopdevice];
	tq = &dv->hdevqueue;
	if (tq->c_cc == 0) {
		x = splsx(gb->g_int_lvl);
		while (tq->c_cc == 0) {
			if (uio->uio_fpflags&FNDELAY)
				break;
			dv->sleep = TRUE;
			y = splx(gb->g_int_lvl);
			splsx(x);
			sleep((caddr_t)dv, HILIPRI);
			x = splsx(gb->g_int_lvl);
			splx(y);
		}
		splsx(x);
	}
	while (uio->uio_resid!=0 && err==0) {
		if (uio->uio_resid >= CBSIZE) {
			register n;
			register struct cblock *cp;

			cp = getcb(tq);
			if (cp == NULL)
				break;
			n = min(uio->uio_resid, cp->c_last - cp->c_first);
			err = uiomove(&cp->c_data[cp->c_first],n,UIO_READ, uio);
			putcf(cp);
		} else {
			register c;

			if ((c = getc(tq)) < 0)
				break;
			err = ureadc(c, uio);
		}
	}
	return(err);
}


nimitz_select(dev, which)
{
	register struct hilrec *g;
	register int loopdevice = (dev&H_HIL_DEVICE)>>4;
	register struct devicerec *dv;
	
	g = which_hil(dev);

	dv = &g->loopdevices[loopdevice];
	switch(which) {
	case FREAD:
		if (dv->hdevqueue.c_cc)
			return(1);
		hilselqueue(&dv->hil_sel);
		break;
	}
	return(0);
}
