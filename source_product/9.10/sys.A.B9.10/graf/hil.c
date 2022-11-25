/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/graf/RCS/hil.c,v $
 * $Revision: 1.3.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 13:35:12 $
 */

#ifndef lint
    static char *hil_c_revision = "@(#) hil.c $Revision: 1.3.83.4 $ $Date: 93/12/09 13:35:12 $ $Author: marshall $";
#endif

#include "../h/errno.h"
#include "../h/param.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/user.h"
#include "../graf/hil.h"
#include "../graf.300/hilioctl.h"
#ifdef __hp9000s800
#   include "../graf.300/gr_data.h"
#   include "../graf.300/graph02.h"
#endif

#define UCS		unsigned char *
#define UC_NULL		(UCS)0

extern int selwait;

#ifdef __hp9000s300
#   define LOOPBUSY_WAIT
#   define LOOPBUSY_DONE
#   define HIL_ENTER(lu)
#endif
#ifdef __hp9000s800
    LOOPBUSY_WAIT(g)
	struct hilrec *g;
    {
	/* Wait to get loopbusy semaphore */
	while (g->loopbusy) {
	    g->waiting_for_loop = TRUE;
	    sleep((caddr_t)&(g->loopbusy), PZERO+1);
	}
	g->loopbusy = TRUE;
    }

    LOOPBUSY_DONE(g)
	struct hilrec *g;
    {
	g->loopbusy = FALSE;
	if (g->waiting_for_loop)
	    wakeup((caddr_t)&(g->loopbusy));
    }

#   define HIL_ENTER(lu) { \
	struct gr_data *grp = &graph_data[lu]; \
	if (grp->graph_service) \
	    (*grp->graph_service)(lu, GRAPHCARD_HIL_WANTED); \
    }
#endif

unsigned char hil_nullstr[] = "\0";

hil_open(dev)							    /* ENTRY */
{
    register struct hilrec *g;
    register int loopdevice = DEV_TO_LOOPDEV(dev);
    register struct devicerec *dv;
    register struct cblock *cp;
    unsigned char data, mask;
    int lu;

    g  = which_hil(dev);
    lu = which_lu(dev, g);

    HIL_ENTER(lu);

#   ifdef __hp9000s800
	if (lu >= num_hil)
	    return(ENXIO);
#   endif

    if ((g == NULL) || (loopdevice > H_LMAXDEVICES) || (loopdevice == 0))
	return(ENXIO);
    if (loopdevice > g->num_ldevices)
	return(ENXIO);

    dv = &g->loopdevices[loopdevice];
    if (dv->open_flag)
	return(EBUSY);

    /* Check to see if the HIL link failed its last reconfiguration */
    hil_cmd(lu, H_READLSTAT, hil_nullstr, 0, &data, TRUE);
    if (data & 0x80)
	return(ENXIO);

    dv->open_flag = TRUE;

    /* Flush out this devices buffer */
    while ((cp = getcb(&dv->hdevqueue)) != NULL)
	putcf(cp);

    /* Enable HIL interrupts. */
    hil_cmd(g, H_INTON, UC_NULL, 0, UC_NULL);

    /* Put device in raw mode. */
    mask = 1 << (loopdevice - 1);
    if (mask & g->current_mask) {
	/* Clear the autorepeat register. */
	data = 0;
	hil_cmd(lu, 0x87, &data, 1, &data, FALSE);
	g->current_mask ^= mask;

	/* Set the cooked keyboard mask. */
	hil_cmd(g, H_WRITEKBDS, &g->current_mask, 1, &data, FALSE);
    }

    return(0);
}

hil_close(dev)							    /* ENTRY */
{
    register struct hilrec *g;
    register int lu, loopdevice = DEV_TO_LOOPDEV(dev);
    register struct devicerec *dv;
    unsigned char data, mask;

    g  = which_hil(dev);
    lu = which_lu(dev, g)

    HIL_ENTER(lu);

    dv = &g->loopdevices[loopdevice];
    dv->open_flag = FALSE;

#   ifdef __hp9000s800
	/*
	 * Disable autorepeat on device so keyboard driver
	 * does not get confused.
	 */
	LOOPBUSY_WAIT(g);
	hil_ldcmd(g, (unsigned char)loopdevice, ioc_cmd, NULL, 0);
	LOOPBUSY_DONE(g);
#   endif

    /* Put device in cooked mode if necessary. */
    mask = 1 << (loopdevice - 1);
    if (mask & g->cooked_mask) {
	g->current_mask ^= mask;

	/* Set the cooked keyboard mask. */
	hil_cmd(g, H_WRITEKBDS, &g->current_mask, 1, &data, FALSE);
    }

    return(0);
}

hil_read(dev, uio)						    /* ENTRY */
    register struct uio *uio;
{
    register x, y, lu;
    register struct clist *tq;
    register struct hilrec *g;
    register struct hilbuf *gb;
    register struct devicerec *dv;
    register int loopdevice = DEV_TO_LOOPDEV(dev);
    int err = 0;
    struct a {
	int fdes;
    };

    g  = which_hil(dev);
    lu = which_lu(dev, g);

    /* Set up the buffer and queue pointers */
    gb = &g->gb;
    dv = &g->loopdevices[loopdevice];
    tq = &dv->hdevqueue;
    if (tq->c_cc == 0) {
#	ifdef __hp9000s300
	    x = splsx(gb->g_int_lvl);
#	endif
#	ifdef __hp9000s800
	    x = spl6();
#	endif

	while (tq->c_cc == 0) {
		if (uio->uio_fpflags&FNDELAY)
		    break;
		dv->sleep = TRUE;

#		ifdef __hp9000s300
		    y = splx(gb->g_int_lvl);
		    splsx(x);
#		endif

		sleep((caddr_t)dv, HILIPRI);

#		ifdef __hp9000s300
		    x = splsx(gb->g_int_lvl);
		    splx(y);
#		endif
	}
#	ifdef __hp9000s300
	    splsx(x);
#	endif
#	ifdef __hp9000s800
	    splx(x);
#	endif
    }
    while (uio->uio_resid!=0 && err==0) {
	register n;

	if (uio->uio_resid >= CBSIZE) {
	    register struct cblock *cp;

	    cp = getcb(tq);
	    if (cp == NULL)
		break;
	    n = min((u_int)(uio->uio_resid),
		    (u_int)(cp->c_last - cp->c_first));
	    err = uiomove(&cp->c_data[cp->c_first], n, UIO_READ, uio);
	    putcf(cp);
	}
	else {
	    if ((n = getc(tq)) < 0)
		break;
	    err = ureadc(n, uio);
	}
    }

    return(err);
}

/*ARGSUSED*/
hil_ioctl(dev, cmd, arg, mode)					    /* ENTRY */
    register unsigned char *arg;
{
    register struct hilrec *g;
    register int lu, loopdevice = DEV_TO_LOOPDEV(dev);
    register int ioc_size = (cmd & H_IOCSIZE)>>16;
    register unsigned char ioc_cmd = (cmd & H_IOCCMD);
    register short index;
    int err = 0;

    g  = which_hil(dev);
    lu = which_lu(dev, g);

    HIL_ENTER(lu);

    if (loopdevice > g->num_ldevices)
	return(ENXIO);

    LOOPBUSY_WAIT(g);

    g->response_length = 0;

    /* Execute the specified command */
    switch (cmd) {
	case HILID:	/* Identify and Describe */
	case HILPST:	/* Perform Self Test */
	case HILRN:	/* Report Name */
	case HILRS:	/* Report Status */
	case HILED:	/* Extended Describe */
	case HILSC:	/* Report Security Code */
	    /*
	     * Send the command to the HIL loop and wait for the
	     * selected device to respond with data.
	     */
#	    ifdef __hp9000s300
		hil_lcommand(g, loopdevice, ioc_cmd);
#	    endif
#	    ifdef __hp9000s800
		err = hil_ldcmd(g, (unsigned char)loopdevice, ioc_cmd, arg, 0);
		if (err)
		    break;
#	    endif

	    for (index=0; index<g->response_length; index++)
		arg[index] = g->loop_response[index];
	    break;
	case HILRR:	/* Read Register */
	    /* The register number must be less than 128 */
	    if (arg[0] > 127)
		return(EINVAL);

	    /*
	     * Send the read register command to the loop device
	     * and wait for the data.
	     */
#	    ifdef __hp9000s300		
		hil_ldcmd(g, loopdevice, ioc_cmd, arg, 1);
#	    endif
#	    ifdef __hp9000s800
		err = hil_ldcmd(g, (unsigned char)loopdevice, ioc_cmd, arg, 1);
#	    endif

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
#	    ifdef __hp9000s300
		hil_lcommand(g, loopdevice, ioc_cmd);
#	    endif
#	    ifdef __hp9000s800
		err = hil_ldcmd(g, (unsigned char)loopdevice, ioc_cmd, arg, 0);
#	    endif
	    break;
	default:
	    /* Check for write register command. */
	    if ((cmd&0xc000ffff) == (IOC_IN|'h'<<8|0x07)) {
		/* Write Register */
		hil_ldcmd(g, loopdevice, ioc_cmd, arg, ioc_size);
		break;
	    }
#	ifdef __hp9000s300
	    else if ((cmd&0x0000ff00) == ('h'<<8) &&
		     (ioc_cmd >= 0x80 && ioc_cmd <= 0xEF)) {
		    /* 
		     * Device Specific command 
		     * Send the command and any data to the HIL loop
		     * and wait for the selected device to respond with
		     * data.
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
		    if (g->response_length) {
			if (ioc_size > g->response_length) {
			    bcopy((caddr_t)g->loop_response,
				  (caddr_t)&arg[1], g->response_length);
			 }
			 else return(EINVAL);
		    }
		    break;
		}
#	endif /* __hp9000s300 */

	/* Unexpected ioctl() command */
	err = EINVAL;
	break;
    }

    /* Did they swipe the device from us after we started?? */
    if (loopdevice > g->num_ldevices)
	return(ENXIO);

    LOOPBUSY_DONE(g);
    return(err);
}

hil_select(dev, which)						    /* ENTRY */
{
    register struct hilrec *g;
    register int lu, loopdevice = DEV_TO_LOOPDEV(dev);
    register struct devicerec *dv;
	
    g  = which_hil(dev);
    lu = which_lu(dev, g);

    dv = &g->loopdevices[loopdevice];
    switch (which) {
	case FREAD:
	    if (dv->hdevqueue.c_cc)
		return(1);
	    hilselqueue(&dv->hil_sel);
	    break;
    }
    return(0);
}

hil_buffer(g)							    /* ENTRY */
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

hilselqueue(hilsel)						    /* ENTRY */
    struct hilselect *hilsel;
{
    register struct proc *p;

    if ((p = hilsel->hil_selp) && (p->p_wchan == (caddr_t) &selwait))
	 hilsel->hil_selflag |= HSEL_COLL;
    else hilsel->hil_selp = u.u_procp;

    return(0);
}

hilselwakeup(hilsel)						    /* ENTRY */
    struct hilselect *hilsel;
{
    if (hilsel->hil_selp) {
	selwakeup(hilsel->hil_selp, hilsel->hil_selflag & HSEL_COLL);
	hilsel->hil_selp = 0;
	hilsel->hil_selflag &= ~HSEL_COLL;
    }

    return(0);
}
