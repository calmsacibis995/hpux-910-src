/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/srm.c,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:18:37 $
 */
/* HPUX_ID: @(#)srm.c	55.1		88/12/23 */

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
#include "../h/buf.h"
#include "../h/systm.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../s200io/bootrom.h"
#include "../wsio/intrpt.h"
#include "../h/errno.h"
#include "../h/conf.h"
#include "../h/uio.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../s200io/srm.h"

#undef timeout /* Use real timeout */

struct srm_table *srm_device_list = (struct srm_table *)0;

struct srm_table *srm_get_dev_entry();

/* Forward Declarations */

void srm_read_timeout();

srm629_open(dev, flag)
    dev_t dev;
    int flag;
{

    if (srm_get_dev_entry(m_selcode(dev)) == 0)
	return(ENXIO);

    cdevsw[major(dev)].d_flags |= C_EVERYCLOSE;
    return(0);
}

srm629_close(dev, flag)
    dev_t dev;
    int flag;
{
    struct srm_table *dev_entry;

    if ((dev_entry = srm_get_dev_entry(m_selcode(dev))) == 0)
	return(EIO);

    /* check to see if I have the lock, if so remove it */

    if (dev_entry->srm_pid == u.u_procp->p_pid) {
	dev_entry->srm_pid = 0;
	dev_entry->srm_revision = 0;
	srm_set_ctl(dev_entry); /* Clear "This Is An Srm" indicator */
	dev_entry->srm_regs->interrupt = 0x00; /* Disable interrupts */
	wakeup((caddr_t)dev_entry);
    }
}

srm629_ioctl(dev,request,arg)
    int dev,request;
    register int *arg;
{
    register struct srm_table *dev_entry;
    register struct srm_reg *srm_regs;
    register u_char *s;
    register int i;
#ifdef SRM_MEASURE
    struct srmstat *stats;
#endif

    if ((dev_entry = srm_get_dev_entry(m_selcode(dev))) == 0)
	return(EIO);

    srm_regs = dev_entry->srm_regs;

    switch(request){

    case SRMLOCK:
	if (dev_entry->srm_pid != 0) {

	    /* if already locked by me return invalid error */

	    if (dev_entry->srm_pid == u.u_procp->p_pid)
		return(EINVAL);

	    /* sleep waiting for lock to free up. */

	    while (dev_entry->srm_pid !=0)
		sleep((caddr_t)dev_entry,PZERO+1);

	}

	/* not locked! quick lock it! */

	dev_entry->srm_pid = u.u_procp->p_pid;

	/* Reset queues -- We don't care about success.  */
	/* We go ahead and enable interrupts anyway, in  */
	/* case the card recovers.                       */

	srm_reset_rcv_queues(dev_entry);

	dev_entry->srm_rcv_packets = 0;
	dev_entry->srm_partial_read = 0;
	dev_entry->srm_rcv_bufhead = dev_entry->srm_rcv_buffer;
	dev_entry->srm_rcv_buftail = dev_entry->srm_rcv_buffer;

	/* Enable interrupts */

	srm_regs->interrupt = 0x80;

	break;

    case SRMUNLOCK:

	/* If my lock unlock it and wakeup any sleepers; else invalid error */

	if (dev_entry->srm_pid != u.u_procp->p_pid) {

	    if (dev_entry->srm_pid == 0)
		return(EINVAL);
	    else
		return(EPERM);
	}

	/* unlock it */

	dev_entry->srm_pid = 0;

	/* Clear THIS_IS_AN_SRM flag */

	dev_entry->srm_revision = 0;
	srm_set_ctl(dev_entry);

	/* Disable Interrupts */

	srm_regs->interrupt = 0x00;

	/* wakeup any sleepers */

	wakeup((caddr_t)dev_entry);
	break;

    case SRMCHGTMO:

	 /* Make sure we have lock */

	if (dev_entry->srm_pid != u.u_procp->p_pid) {
	     if (dev_entry->srm_pid == 0)
		   return(EINVAL);
	     else
		   return(EPERM);
	}

	/* reset the timeout value from the default to what the usr wants. */

	dev_entry->srm_timeout = *arg;
	break;

    case SRMSETCTL:

	 /* Make sure we have lock */

	if (dev_entry->srm_pid != u.u_procp->p_pid) {
	     if (dev_entry->srm_pid == 0)
		   return(EINVAL);
	     else
		   return(EPERM);
	}

	/* Set or Clear the THIS_IS_AN_SRM bit */

	dev_entry->srm_revision = *arg & 0xff;
	srm_set_ctl(dev_entry);
	break;

    case SRMGETNODE:

	*arg = dev_entry->srm_node;
	break;

    case SRMRESET:

	/* Make sure we have lock or we are superuser */

	if (dev_entry->srm_pid != u.u_procp->p_pid && !suser()) {
	     if (dev_entry->srm_pid == 0)
		   return(EINVAL);
	     else
		   return(EPERM);
	}

	if (srm_reset(dev_entry) == -1)
	    return(EIO);
	break;

    case SRMSETQSIZE:

	 /* Make sure we have lock */

	if (dev_entry->srm_pid != u.u_procp->p_pid) {
	     if (dev_entry->srm_pid == 0)
		   return(EINVAL);
	     else
		   return(EPERM);
	}

	/* Check the argument to make sure it is not */
	/* too small. If not superuser, make sure it */
	/* is not too big.                           */

	i = *arg;
	if (i < 1024 || (!suser() && i > 16384))
	    return(EINVAL);

	/* make sure size is even */

	if ((i & 0x1) != 0)
	    i++;

	/* Try to allocate a new buffer */

	s = (u_char *)sys_memall(i);
	if (s == (u_char *)0)
	    return(ENOMEM);

	/* Disable interrupts from this card */

	srm_regs->interrupt = 0x00;

	/* Reset queues -- We don't care about success.  */
	/* We go ahead and enable interrupts anyway, in  */
	/* case the card recovers.                       */

	srm_reset_rcv_queues(dev_entry);

	/* Free up old buffer */

	sys_memfree(dev_entry->srm_rcv_buffer,
		    dev_entry->srm_rcv_bufsize);

	dev_entry->srm_rcv_packets = 0;
	dev_entry->srm_partial_read = 0;
	dev_entry->srm_rcv_bufsize = i;
	dev_entry->srm_rcv_buffer  = s;
	dev_entry->srm_rcv_bufhead = s;
	dev_entry->srm_rcv_buftail = s;

	/* Enable interrupts */

	srm_regs->interrupt = 0x80;
	break;

    case SRMGETQSIZE:
	*arg = dev_entry->srm_rcv_bufsize;
	break;

#ifdef SRM_MEASURE
    case SRMSTATS:
	stats = (struct srmstat *)arg;

	stats->sem_try_cnt  = dev_entry->sem_try_cnt;
	stats->sem_fail_cnt = dev_entry->sem_fail_cnt;
	stats->max_rcvqsize = dev_entry->max_rcvqsize;
	stats->rcv_fill_cnt = dev_entry->rcv_fill_cnt;
	stats->int_cnt      = dev_entry->int_cnt;
	stats->pkt_cnt      = dev_entry->pkt_cnt;
	stats->max_int_packs= dev_entry->max_int_packs;
	stats->max_int_time = dev_entry->max_int_time;
	stats->datahold_cnt = dev_entry->datahold_cnt;
	stats->error_cnt    = dev_entry->error_cnt;
	stats->last_error   = dev_entry->last_error;
	i = srm_command(dev_entry->srm_regs,SRM_CRC,0);
	if (i == -1)
	    return(EIO);
	stats->rcv_crc_cnt = i;

	i = srm_command(dev_entry->srm_regs,SRM_OVF,0);
	if (i == -1)
	    return(EIO);
	stats->rcv_bufovf_cnt = i;

	i = srm_command(dev_entry->srm_regs,SRM_TXRETRY,0);
	if (i == -1)
	    return(EIO);
	stats->tx_retry_cnt = i;
	break;
#endif

    default:

	/* wrong! */

	return(EINVAL);
	break;
    }

    return(0);
}

srm629_read(dev, uio)
    dev_t dev;
    register struct uio *uio;
{
    register struct srm_table *dev_entry;
    register int i;
    register int n;
    register u_char *head_ptr;
    int err;
    int full_pack_flag;

    if ((dev_entry = srm_get_dev_entry(m_selcode(dev))) == 0)
	return(EIO);

    /* if not owned by me return permission error */

    if (dev_entry->srm_pid == 0 || dev_entry->srm_pid != u.u_procp->p_pid)
	return(EPERM);

    i = splx(SRM_INT_LVL);
    n = dev_entry->srm_rcv_packets;
    splx(i);

    /* Do we have a packet? */

    if (n == 0) {

	/* No. Return appropriate value if non-blocking IO */

	if (uio->uio_fpflags & (FNDELAY|FNBLOCK)) {
	    if (uio->uio_fpflags & FNDELAY)
		return(0);
	    else
		return(EAGAIN);
	}

	/* Block waiting for a packet */

	i = CRIT();
	while (dev_entry->srm_rcv_packets == 0) {

	    dev_entry->srm_flags |= SRM_RDSLEEP;

	    /* Schedule a timeout if srm_timeout != 0 */

	    if (dev_entry->srm_timeout != 0) {
		timeout(srm_read_timeout,dev_entry,
			(dev_entry->srm_timeout * HZ) / 10);
	    }

	    /* We sleep uninterruptable so that a request */
	    /* to an srm will not leave the srm in a bad  */
	    /* state (it could be if a srm utility was    */
	    /* killed during a read/write). This argument */
	    /* is not entirely valid, but since the last  */
	    /* version of the driver did this, it was     */
	    /* decided to not to change it.               */

	    sleep((caddr_t)&dev_entry->srm_rcv_packets, PSWP);

	    /* Check for timeout */

	    if (dev_entry->srm_flags & SRM_TIMEOUT) {
		dev_entry->srm_flags &= ~(SRM_TIMEOUT|SRM_RDSLEEP);
		UNCRIT(i);
		return(0);
	    }
	    else
		untimeout(srm_read_timeout,dev_entry);
	}
	UNCRIT(i);
    }

    /* Loop, taking bytes from queue and copying them */
    /* to user space.                                 */

    head_ptr = dev_entry->srm_rcv_bufhead;
    n = *((short *)head_ptr);
    i = dev_entry->srm_partial_read;
    head_ptr += (i + 2);
    n -= i;
    if (uio->uio_resid < n) {
	n = uio->uio_resid;
	full_pack_flag = FALSE;
    }
    else
	full_pack_flag = TRUE;

    if ((head_ptr + n) > (dev_entry->srm_rcv_buffer + dev_entry->srm_rcv_bufsize)) {
	i = (dev_entry->srm_rcv_buffer - head_ptr) + dev_entry->srm_rcv_bufsize;
	err = uiomove(head_ptr,i,UIO_READ,uio);
	if (err != 0)
	    return(err);

	head_ptr = dev_entry->srm_rcv_buffer;
    }
    else
	i = 0;

    err = uiomove(head_ptr,n-i,UIO_READ,uio);
    if (err != 0)
	return(err);

    if (full_pack_flag == FALSE)
	dev_entry->srm_partial_read += n;
    else {
	dev_entry->srm_partial_read = 0;

	head_ptr += (n - i);
	if ( (((int)head_ptr) & 0x1) != 0)
	    head_ptr++;

	if (head_ptr == (dev_entry->srm_rcv_buffer + dev_entry->srm_rcv_bufsize))
	    head_ptr = dev_entry->srm_rcv_buffer;

	i = CRIT();
	dev_entry->srm_rcv_bufhead = head_ptr;
	dev_entry->srm_rcv_packets--;

	/* Since we just freed up some space on the queue, check to see    */
	/* if we are holding any data on the card. If so, call srm_receive */
	/* directly to try to transfer the data.                           */

	if (dev_entry->srm_flags & SRM_DATAHOLD) {

	    dev_entry->srm_flags |= SRM_RCVTRG;
	    dev_entry->srm_flags &= ~SRM_DATAHOLD;
	    UNCRIT(i);
	    srm_receive(dev_entry);
	}
	else
	    UNCRIT(i);
    }
    return(0);
}

void
srm_read_timeout(dev_entry)
    register struct srm_table *dev_entry;
{
    register int i;

    i = CRIT();
    dev_entry->srm_flags |= SRM_TIMEOUT;
    UNCRIT(i);
    wakeup((caddr_t)&dev_entry->srm_rcv_packets);
    return;
}

srm629_write(dev, uio)
    dev_t dev;
    struct uio *uio;
{
    register struct srm_table *dev_entry;
    register struct srm_reg *srm_regs;
    register int pkt_size;
    register int i;
    register int addr;
    register int ctrl_qspace;
    register int data_qspace;
    register u_char  *pkt_data;
    register u_char  *tx_data_qfill;
    u_char  *tx_ctrl_qfill;
    u_char  *tx_data_qempty;
    u_char  *tx_ctrl_qempty;
    u_char  *tx_data_qend;

    if ((dev_entry = srm_get_dev_entry(m_selcode(dev))) == 0)
	return(EIO);

    /* if not owned by me return permission error */

    if (dev_entry->srm_pid == 0 || dev_entry->srm_pid != u.u_procp->p_pid)
	return(EPERM);

    pkt_size = uio->uio_resid;

    /* Check to make sure packet is not too large or too small */

    if (pkt_size > MAX_SRM_PACK_SIZE || pkt_size < MIN_SRM_PACK_SIZE)
	return(EINVAL);

    /* Copy the packet data from user space to the transmit buffer. */
    /* We do this instead of copying directly to the card because   */
    /* we have to write to the card 1 byte at a time. The overhead  */
    /* of uwritec()/fubyte() would cause that method to not perform */
    /* as well.                                                     */

    pkt_data = dev_entry->srm_tx_buffer;
    i = uiomove(pkt_data,pkt_size,UIO_WRITE,uio);
    if (i != 0)
	return(i);

    srm_regs = dev_entry->srm_regs;
    tx_ctrl_qfill = SRM_POINTER(srm_regs,dev_entry->tx_ctrl_qfill_ptr);
    tx_data_qfill = SRM_POINTER(srm_regs,dev_entry->tx_data_qfill_ptr);
    for (;;) {

	/* See if there is room on the transmit control and data */
	/* queues for this packet.                               */

#ifdef SRM_MEASURE
	if ((i = srm_crit(&srm_regs->semaphore,dev_entry)) == -1)
#else
	if ((i = srm_crit(&srm_regs->semaphore)) == -1)
#endif
	    return(EIO);

	tx_ctrl_qempty = SRM_POINTER(srm_regs,dev_entry->tx_ctrl_qempty_ptr);
	tx_data_qempty = SRM_POINTER(srm_regs,dev_entry->tx_data_qempty_ptr);
	srm_regs->semaphore = 0;
	UNCRIT(i);

	/* Compute room available on tx control queue */

	ctrl_qspace = tx_ctrl_qfill - tx_ctrl_qempty;
	if (ctrl_qspace < 0)
	    ctrl_qspace = ((-ctrl_qspace - 2) >> 1);
	else
	    ctrl_qspace = (dev_entry->tx_ctrl_qsize - 2 - ctrl_qspace) >> 1;

	/* Compute room available on tx data queue */

	data_qspace = tx_data_qfill - tx_data_qempty;
	if (data_qspace < 0)
	    data_qspace = ((-data_qspace - 4) >> 1);
	else
	    data_qspace = (dev_entry->tx_data_qsize - 4 - data_qspace) >> 1;

	/* See if there is enough room to transmit packet. Break out of */
	/* the loop if there is enough room.                            */

	if (ctrl_qspace >= 4 && data_qspace >= pkt_size)
	    break;

	/* Wait for room. Schedule a timeout and then go to sleep */

	i = CRIT();
	timeout(wakeup,&dev_entry->tx_data_qsize,1);

	/* We sleep uninterruptable so that a request */
	/* to an srm will not leave the srm in a bad  */
	/* state (it could be if a srm utility was    */
	/* killed during a read/write). This argument */
	/* is not entirely valid, but since the last  */
	/* version of the driver did this, it was     */
	/* decided to not to change it.               */

	sleep((caddr_t)&dev_entry->tx_data_qsize,PSWP);
	UNCRIT(i);
    }

    /* Transmit the packet now, since room is available. */

    /* Put data on transmit data queue */

    tx_data_qend = dev_entry->tx_data_queue + dev_entry->tx_data_qsize;
    while (pkt_size-- > 0) {
	*tx_data_qfill = *pkt_data++;
	tx_data_qfill += 2;
	if (tx_data_qfill == tx_data_qend)
	    tx_data_qfill = dev_entry->tx_data_queue;
    }

    /* Put an eof packet on transmit control queue */

    addr = (tx_data_qfill - ((u_char *)srm_regs)) >> 1;

    *tx_ctrl_qfill = addr & 0xff;
    tx_ctrl_qfill += 2;
    *tx_ctrl_qfill = addr >> 8;
    tx_ctrl_qfill += 2;
    *tx_ctrl_qfill = 0x05;
    tx_ctrl_qfill += 2;
    *tx_ctrl_qfill = 0x01;
    tx_ctrl_qfill += 2;

    if (tx_ctrl_qfill == (dev_entry->tx_ctrl_queue + dev_entry->tx_ctrl_qsize))
	tx_ctrl_qfill = dev_entry->tx_ctrl_queue;

    /* Update fill pointers on card */

#ifdef SRM_MEASURE
    if ((i = srm_crit(&srm_regs->semaphore,dev_entry)) == -1)
#else
    if ((i = srm_crit(&srm_regs->semaphore)) == -1)
#endif
	return(EIO);

    *(dev_entry->tx_data_qfill_ptr) = addr & 0xff;
    *(dev_entry->tx_data_qfill_ptr + 2) = addr >> 8;

    addr = (tx_ctrl_qfill - ((u_char *)srm_regs)) >> 1;

    *(dev_entry->tx_ctrl_qfill_ptr) = addr & 0xff;
    *(dev_entry->tx_ctrl_qfill_ptr + 2) = addr >> 8;

    /* Release semaphore and return success */

    srm_regs->semaphore = 0;
    UNCRIT(i);

    return(0);
}

srm629_select(dev, which)
    dev_t dev;
    int which;
{
    struct srm_table *dev_entry;
    register struct proc *p;
    register int i;

    if (which != FREAD)
	return(0);

    if ((dev_entry = srm_get_dev_entry(m_selcode(dev))) == 0)
	    return(1);

    i = CRIT();
    if (dev_entry->srm_rcv_packets != 0) {
	UNCRIT(i);
	return(1);
    }

    if ((p = dev_entry->srm_rsel) && (p->p_wchan == (caddr_t) &selwait))
	dev_entry->srm_flags |= SRM_RCOLL;
    else
	dev_entry->srm_rsel = u.u_procp;

    UNCRIT(i);

    return(0);
}

srm_command(srm_regs,command,data)
    register struct srm_reg *srm_regs;
    int command;
    int data;
{
    register int int_flag;
    register int try_count = 100;

    /* disable interrupts if enabled */

    int_flag = srm_regs->interrupt & 0x80;
    if (int_flag)
	srm_regs->interrupt = 0x00;

    srm_regs->data = data;
    srm_regs->command = command;

    while (try_count != 0) {
	snooze(100);
	if (srm_regs->command == 0)
	    break;
	try_count--;
    }

    /* Re-enable interrupts if we disabled them */

    if (int_flag)
	srm_regs->interrupt = 0x80;

    if (try_count == 0)
	return(-1);

    return((int)srm_regs->data);
}

srm_isr(inf)
    struct interrupt *inf;
{
    register struct srm_table *dev_entry = (struct srm_table *)inf->temp;
    register struct srm_reg *srm_regs;
    register int int_cond;
    register int error_code;
    register int i;
#ifdef SRM_MEASURE
    struct timeval btime;
    struct timeval etime;

    get_precise_time(&btime);
#endif

    srm_regs = dev_entry->srm_regs;

    /* Get interrupt condition register             */
    /* Note: this clears the interrupting condition */

    int_cond = srm_regs->int_cond;
    if (int_cond & SRMINT_ERROR) {
	error_code = srm_regs->error_code;
	srm_regs->error_code = 0;
#ifdef SRM_MEASURE
	dev_entry->error_cnt++;
	dev_entry->last_error = error_code;
#endif
    }

    if (int_cond & SRMINT_RCVDATA) {

	/* Call srm_receive to do input processing */

	i = CRIT();
	if (dev_entry->srm_flags & (SRM_RCVTRG|SRM_DATAHOLD))
	    UNCRIT(i);
	else {
	    dev_entry->srm_flags |= SRM_RCVTRG;
	    UNCRIT(i);
	    srm_receive(dev_entry);
	}
    }

#ifdef SRM_MEASURE
    get_precise_time(&etime);

    int_cond =   (etime.tv_sec * 1000 + etime.tv_usec / 1000)
	       - (btime.tv_sec * 1000 + btime.tv_usec / 1000);

    if (int_cond > dev_entry->max_int_time)
	dev_entry->max_int_time = int_cond;
#endif

    return;
}

srm_receive(dev_entry)
    register struct srm_table *dev_entry;
{
    register int npackets;
    register int srm_rcv_size;
    register int pkt_size;
    register int pkts_to_move;
    register int i;
    register int j;
    register struct srm_reg *srm_regs;
    register u_char  *rx_data_qempty;
    register u_char  *srm_rcv_buftail;
    u_char  *rx_ctrl_qempty;
    u_char  *rx_ctrl_qfill;
    u_char  *rx_data_qfill;
    u_char  *rx_pkt_end;
    u_char  *rx_data_qend;
    u_char  *rx_ctrl_qend;
    u_char  *rx_buf_end;

    srm_regs = dev_entry->srm_regs;

#ifdef SRM_MEASURE
    dev_entry->int_cnt++;
#endif

    /* Get receive control block and data pointers */

#ifdef SRM_MEASURE
    if ((i = srm_crit(&srm_regs->semaphore,dev_entry)) == -1) {
#else
    if ((i = srm_crit(&srm_regs->semaphore)) == -1) {
#endif
	i = CRIT();
	dev_entry->srm_flags &= ~SRM_RCVTRG;
	UNCRIT(i);
	return;
    }

    rx_ctrl_qfill = SRM_POINTER(srm_regs,dev_entry->rx_ctrl_qfill_ptr);
    rx_data_qfill = SRM_POINTER(srm_regs,dev_entry->rx_data_qfill_ptr);
    srm_regs->semaphore = 0;

    UNCRIT(i);

    rx_ctrl_qempty = SRM_POINTER(srm_regs,dev_entry->rx_ctrl_qempty_ptr);
    rx_data_qempty = SRM_POINTER(srm_regs,dev_entry->rx_data_qempty_ptr);

    /* Compute room available on the srm receive buffer */

    srm_rcv_buftail = dev_entry->srm_rcv_buftail;
    srm_rcv_size = srm_rcv_buftail - dev_entry->srm_rcv_bufhead;
    if (srm_rcv_size < 0)
	srm_rcv_size = -srm_rcv_size - 1;
    else
	srm_rcv_size = dev_entry->srm_rcv_bufsize - 1 - srm_rcv_size;

    /* Get pointers to end of queues for comparison during transfer loop */

    rx_data_qend = dev_entry->rx_data_queue + dev_entry->rx_data_qsize;
    rx_ctrl_qend = dev_entry->rx_ctrl_queue + dev_entry->rx_ctrl_qsize;
    rx_buf_end = dev_entry->srm_rcv_buffer + dev_entry->srm_rcv_bufsize;

    /* Compute # of packets received */

    npackets = rx_ctrl_qfill - rx_ctrl_qempty;
    if (npackets < 0)
	npackets += dev_entry->rx_ctrl_qsize;
    npackets >>= 3;

    pkts_to_move = npackets;
    while (pkts_to_move != 0) {

	/* Get pointer to end of packet from control packet */

	rx_pkt_end = SRM_POINTER(srm_regs,rx_ctrl_qempty);

	/* Get size of packet */

	pkt_size = rx_pkt_end - rx_data_qempty;
	if (pkt_size < 0)
	    pkt_size += dev_entry->rx_data_qsize;
	pkt_size >>= 1;

	/* Check to see if we can fit packet in srm receive buffer  */
	/* The reason we add 3 is: 2 for packet length, 1 if packet */
	/* length is odd, since we always move the buffer tail to   */
	/* an even boundary so that the next packet length will be  */
	/* aligned.                                                 */

	if ((pkt_size + 3) > srm_rcv_size) {
#ifdef SRM_MEASURE
	    dev_entry->rcv_fill_cnt++;
#endif
	    break;
	}

	/* Put packet length into buffer */

	*((short *)srm_rcv_buftail) = pkt_size;
	srm_rcv_buftail += 2;
	if (srm_rcv_buftail == rx_buf_end)
	    srm_rcv_buftail = dev_entry->srm_rcv_buffer;

	/* Transfer data from card to srm receive buffer */

	i = pkt_size;
	while (i--  > 0) {
	    *srm_rcv_buftail++ = *rx_data_qempty;
	    rx_data_qempty += 2;
	    if (srm_rcv_buftail == rx_buf_end)
		srm_rcv_buftail = dev_entry->srm_rcv_buffer;
	    if (rx_data_qempty == rx_data_qend)
		rx_data_qempty = dev_entry->rx_data_queue;
	}

	/* eat control packet */

	rx_ctrl_qempty += 8;
	if (rx_ctrl_qempty == rx_ctrl_qend)
	    rx_ctrl_qempty = dev_entry->rx_ctrl_queue;

	/* Decrement # of packets to move */

	pkts_to_move--;

	/* Recompute space remaining in srm buffer */

	if ((pkt_size & 0x1) == 0)
	    srm_rcv_size -= (pkt_size + 2);
	else {
	    srm_rcv_size -= (pkt_size + 3);
	    srm_rcv_buftail++;
	    if (srm_rcv_buftail == rx_buf_end)
		srm_rcv_buftail = dev_entry->srm_rcv_buffer;
	}
    }

    /* Did we get any packets? */

    if (npackets == 0) {

	/* No, just return */

	i = CRIT();
	dev_entry->srm_flags &= ~SRM_RCVTRG;
	UNCRIT(i);
	return;
    }

#ifdef SRM_MEASURE
    dev_entry->pkt_cnt += (npackets - pkts_to_move);
    if (npackets > dev_entry->max_int_packs)
	dev_entry->max_int_packs = npackets;
#endif


    /* Yes, did we move any of them? */

    if (pkts_to_move == npackets) {

	/* No, set DATAHOLD flag, clear trigger and return. */

	i = CRIT();
	dev_entry->srm_flags |= SRM_DATAHOLD;
	dev_entry->srm_flags &= ~SRM_RCVTRG;
	UNCRIT(i);
#ifdef SRM_MEASURE
	dev_entry->datahold_cnt++;
#endif
	return;
    }

    /* Packets were moved, we need to update card pointers */

#ifdef SRM_MEASURE
    if ((i = srm_crit(&srm_regs->semaphore,dev_entry)) == -1) {
#else
	if ((i = srm_crit(&srm_regs->semaphore)) == -1) {
#endif
	i = CRIT();
	dev_entry->srm_flags &= ~SRM_RCVTRG;
	UNCRIT(i);
	return;
    }

    j = (rx_data_qempty - ((u_char *)srm_regs)) >> 1;
    *(dev_entry->rx_data_qempty_ptr) = j & 0xff;
    *(dev_entry->rx_data_qempty_ptr + 2) = j >> 8;

    j = (rx_ctrl_qempty - ((u_char *)srm_regs)) >> 1;
    *(dev_entry->rx_ctrl_qempty_ptr) = j & 0xff;
    *(dev_entry->rx_ctrl_qempty_ptr + 2) = j >> 8;

    /* Release semaphore */

    srm_regs->semaphore = 0;

    /* update rcv info */

    dev_entry->srm_rcv_packets += (npackets - pkts_to_move);
    dev_entry->srm_rcv_buftail = srm_rcv_buftail;

    /* Note: some of these if's are redundant so that the */
    /* semaphore could be released as soon as possible.   */

    /* Did we move all packets? */

    if (pkts_to_move != 0) {

	/* No, set DATAHOLD flag */

	dev_entry->srm_flags |= SRM_DATAHOLD;
#ifdef SRM_MEASURE
	dev_entry->datahold_cnt++;
#endif
    }

    /* Do select and read wakeups */

    if (dev_entry->srm_flags & SRM_RDSLEEP) {
	dev_entry->srm_flags &= ~SRM_RDSLEEP;
	UNCRIT(i);
	i = -1;
	wakeup((caddr_t)&dev_entry->srm_rcv_packets);
    }

    if (dev_entry->srm_rsel) {
	if (i == -1)
	    i = CRIT();
	j = dev_entry->srm_flags & SRM_RCOLL;
	dev_entry->srm_flags &= ~SRM_RCOLL;
	UNCRIT(i);
	i = -1;
	selwakeup(dev_entry->srm_rsel, j);
	dev_entry->srm_rsel = (struct proc *)0;
    }

#ifdef SRM_MEASURE
    j = dev_entry->srm_rcv_bufsize - srm_rcv_size - 1;
    if (j > dev_entry->max_rcvqsize)
	dev_entry->max_rcvqsize = j;
#endif

    if (i == -1)
	i = CRIT();
    dev_entry->srm_flags &= ~SRM_RCVTRG;
    UNCRIT(i);

    return;
}

/*
** linking and inititialization routines
*/

extern int (*make_entry)();

int (*srm_saved_make_entry)();

srm629_make_entry(id, isc)
    int id;
    struct isc_table_type *isc;
{
    struct srm_table *new_dev_entry, *temp_location;
    register struct srm_reg *srm_regs;
    register int i;
    u_char *srm_buffer;
    int my_node;

    if (id == 20+32) {
	if (data_com_type(isc) == 3) {

	    if (isc->int_lvl != SRM_INT_LVL)
		return(io_inform("HP98629 SRM/MUX Interface", isc, SRM_INT_LVL));

	    srm_regs = (struct srm_reg *) ( (caddr_t)isc->card_ptr);

	    /* Check to see if we can obtain semaphore (reset complete) */

#ifdef SRM_MEASURE
	    if (   (my_node = srm_command(srm_regs,SRM_GETNODE,0)) == -1
		|| (i = srm_crit(&srm_regs->semaphore,(struct srm_table *)0)) == -1)  {
#else
	    if (   (my_node = srm_command(srm_regs,SRM_GETNODE,0)) == -1
		|| (i = srm_crit(&srm_regs->semaphore)) == -1)  {
#endif
		return(io_inform("HP98629 SRM/MUX Interface", isc, -2, " ignored; hardware error"));
	    }

	    /* return semaphore */

	    srm_regs->semaphore = 0;
	    UNCRIT(i);

	    /* Allocate srm table entry and srm receive buffer */

	    new_dev_entry = (struct srm_table *)calloc(sizeof(struct srm_table));
	    srm_buffer = (u_char *)sys_memall(SRM_DEF_BUF_SIZE);

	    if (new_dev_entry == (struct srm_table *)0 || srm_buffer == (u_char *)0)
		return(io_inform("HP98629 SRM/MUX Interface", isc, -2, " ignored; no more memory"));
	    else {
		if (srm_command(srm_regs,SRM_SETINTR,SRM_INT_MASK) == -1)
		    return(io_inform("HP98629 SRM/MUX Interface", isc, -2, " ignored; cannot enable interrupts"));

		srm_command(srm_regs,SRM_SETRETRY,100);

		new_dev_entry->srm_regs = srm_regs;
		new_dev_entry->srm_node = my_node;
		new_dev_entry->srm_revision = 0;
		new_dev_entry->srm_rcv_packets = 0;
		new_dev_entry->srm_partial_read = 0;
		new_dev_entry->srm_rcv_bufsize = SRM_DEF_BUF_SIZE;
		new_dev_entry->srm_rcv_buffer  = srm_buffer;
		new_dev_entry->srm_rcv_bufhead = srm_buffer;
		new_dev_entry->srm_rcv_buftail = srm_buffer;
		new_dev_entry->srm_sc   = isc->my_isc;
		new_dev_entry->srm_pid  = 0;
		new_dev_entry->srm_timeout = 600;
		new_dev_entry->srm_flags = 0;
		new_dev_entry->srm_rsel = (struct proc *)0;
		new_dev_entry->srm_next_dev = (struct srm_table *)0;
#ifdef SRM_MEASURE
		new_dev_entry->sem_try_cnt = 0;
		new_dev_entry->sem_fail_cnt = 0;
		new_dev_entry->max_rcvqsize = 0;
		new_dev_entry->rcv_fill_cnt = 0;
		new_dev_entry->int_cnt = 0;
		new_dev_entry->pkt_cnt = 0;
		new_dev_entry->max_int_packs = 0;
		new_dev_entry->max_int_time = 0;
		new_dev_entry->datahold_cnt = 0;
		new_dev_entry->error_cnt = 0;
		new_dev_entry->last_error = 0;
#endif

		/* Install isr */

		isrlink(srm_isr,isc->int_lvl,&srm_regs->interrupt,
			0xc0,0xc0,
			0x0,(int)new_dev_entry);

		/* Set roll call registers */

		srm_set_ctl(new_dev_entry);

		isc->card_type = HP98629;

		srm_get_pointers(new_dev_entry);

		if (srm_device_list == NULL)
			srm_device_list = new_dev_entry;
		else {
		    temp_location = srm_device_list;
		    while (temp_location->srm_next_dev != NULL)
			temp_location = temp_location->srm_next_dev;

		    temp_location->srm_next_dev = new_dev_entry;
		}

		io_inform("HP98629 SRM/MUX Interface", isc, -2, ", Node %d",my_node);
		return(1);
	    }

	}
    }

    return((*srm_saved_make_entry)(id, isc));
}

struct msus (*srm_saved_msus_for_boot)();

/* code for converting a dev_t to a boot ROM msus */

struct msus 
srm_msus_for_boot(blocked, dev)
    char blocked;
    dev_t dev;
{
    extern struct cdevsw cdevsw[];
    register struct msus my_msus;
    register dev_type;
    int maj = major(dev);
    struct srm_table *dev_entry;

    /* check if open routine for this dev is me ? (srm driver) */

    if (blocked || maj >= nchrdev || (cdevsw[maj].d_open != srm629_open))
	return((*srm_saved_msus_for_boot)(blocked, dev));

    /* check if card is present? */

    if ((dev_entry = srm_get_dev_entry(m_selcode(dev)))== 0)
	dev_type = 31;
    else
	dev_type = 1;

    /* 0xe108scba  */

    my_msus.dir_format   /* :3 */ = 7; /* srm */
    my_msus.device_type  /* :5 */ = dev_type; /* special */
    my_msus.vol          /* :4 */ = 0;
    my_msus.unit         /* :4 */ = 8; /* history is wonderful */
    my_msus.sc           /* :8 */ = dev_entry->srm_sc;
    my_msus.ba           /* :8 */ = minor(dev); /* node number */

    return (my_msus);
}

srm629_link()
{
    extern struct msus (*msus_for_boot)();

    srm_saved_make_entry = make_entry;
    make_entry = srm629_make_entry;

    srm_saved_msus_for_boot = msus_for_boot;
    msus_for_boot = srm_msus_for_boot;
}

struct srm_table *
srm_get_dev_entry(sc)
    register int sc;
{
    register struct srm_table *current_entry;

    if (!srm_device_list)
	return((struct srm_table *)0);

    if (sc == 0)
	return(srm_device_list);

    current_entry = srm_device_list;

    while ((current_entry != NULL) && (current_entry->srm_sc != sc))
	current_entry = current_entry->srm_next_dev;

    if (current_entry)
	return(current_entry);
    else
	return((struct srm_table *)0);
}

srm_get_pointers(dev_entry)
    register struct srm_table *dev_entry;
{
    register struct srm_reg *srm_regs;
    register u_char *primary_qptr;
    register u_char *qptr;
    register int tx_data_qsize;

    srm_regs = dev_entry->srm_regs;

    primary_qptr = SRM_POINTER(srm_regs,&srm_regs->dsdp_low);
    primary_qptr = SRM_POINTER(srm_regs,primary_qptr + DSDP_P0_OFFSET);

    /* Get Transmit Queue Information */

    qptr = primary_qptr + TXBUFF;

    tx_data_qsize = SRM_DATA(qptr + DATA_AREA + Q_SIZE);

    /* The following is a sanity check. We should never */
    /* panic here.                                      */

    if (tx_data_qsize < MAX_SRM_PACK_SIZE)
	panic("srm card transmit queue too small.\n");
    dev_entry->tx_data_qsize = 2 * tx_data_qsize;

    dev_entry->tx_data_queue = SRM_POINTER(srm_regs,qptr + DATA_AREA + Q_ADDR);
    dev_entry->tx_data_qfill_ptr = qptr + DATA_AREA + Q_FILL;
    dev_entry->tx_data_qempty_ptr = qptr + DATA_AREA + Q_EMPTY;

    dev_entry->tx_ctrl_qsize = 2 * SRM_DATA(qptr + CTRL_AREA + Q_SIZE);
    dev_entry->tx_ctrl_queue = SRM_POINTER(srm_regs,qptr + CTRL_AREA + Q_ADDR);
    dev_entry->tx_ctrl_qfill_ptr = qptr + CTRL_AREA + Q_FILL;
    dev_entry->tx_ctrl_qempty_ptr = qptr + CTRL_AREA + Q_EMPTY;

    /* Get Receive Queue Information */

    qptr = primary_qptr + RXBUFF;

    dev_entry->rx_data_qsize = 2 * SRM_DATA(qptr + DATA_AREA + Q_SIZE);
    dev_entry->rx_data_queue = SRM_POINTER(srm_regs,qptr + DATA_AREA + Q_ADDR);
    dev_entry->rx_data_qfill_ptr = qptr + DATA_AREA + Q_FILL;
    dev_entry->rx_data_qempty_ptr = qptr + DATA_AREA + Q_EMPTY;

    dev_entry->rx_ctrl_qsize = 2 * SRM_DATA(qptr + CTRL_AREA + Q_SIZE);
    dev_entry->rx_ctrl_queue = SRM_POINTER(srm_regs,qptr + CTRL_AREA + Q_ADDR);
    dev_entry->rx_ctrl_qfill_ptr = qptr + CTRL_AREA + Q_FILL;
    dev_entry->rx_ctrl_qempty_ptr = qptr + CTRL_AREA + Q_EMPTY;

    return;
}

#ifdef SRM_MEASURE
srm_crit(sem_ptr,dev_entry)
    register u_char *sem_ptr;
    register struct srm_table *dev_entry;
#else
srm_crit(sem_ptr)
    register u_char *sem_ptr;
#endif
{
    register int tries = 0;
    register int s;

    while (tries++ < 100000) {

	s = CRIT();
	if ((*sem_ptr & 0x80) == 0) {
#ifdef SRM_MEASURE
	    if (   dev_entry != (struct srm_table *)0
		&& tries > dev_entry->sem_try_cnt)
		dev_entry->sem_try_cnt = tries;
#endif
	    return(s);
	}

	/* Let interrupts have a chance while we are looping */

	UNCRIT(s);
    }

#ifdef SRM_MEASURE
    if (dev_entry != (struct srm_table *)0)
	dev_entry->sem_fail_cnt++;
#endif
    printf("SRM driver could not get semaphore.\n");
    return(-1);
}

srm_set_ctl(dev_entry)
    struct srm_table *dev_entry;
{
    register struct srm_reg *srm_regs;

    srm_regs = dev_entry->srm_regs;

    if (dev_entry->srm_revision == 0) {
	srm_regs->is_srm   = 0;
	srm_regs->version  = SRM_MODEL >> 8;
	srm_regs->model    = SRM_MODEL & 0xff;
    }
    else {
	srm_regs->is_srm   = 1;
	srm_regs->version  = dev_entry->srm_revision; /* Software revision */
	srm_regs->model    = SRM_OTHER;   /* computer id flag  */
    }

    return;
}

srm_reset(dev_entry)
    register struct srm_table *dev_entry;
{
    register struct srm_reg *srm_regs;
    register int i;

    /* Disable interrupts */

    srm_regs->interrupt = 0x00;

    /* Reset the Card */

    srm_regs->id_reset = 0x80;

    /* Wait for semaphore */

#ifdef SRM_MEASURE
    if ((i = srm_crit(&srm_regs->semaphore,dev_entry)) == -1)
#else
    if ((i = srm_crit(&srm_regs->semaphore)) == -1)
#endif
	return(-1);

    /* Release semaphore */

    srm_regs->semaphore = 0;
    UNCRIT(i);

    /* Set interrupt mask */

    if (srm_command(srm_regs,SRM_SETINTR,SRM_INT_MASK) == -1)
	return(-1);

    srm_command(srm_regs,SRM_SETRETRY,100);

    /* Set roll call registers appropriately */

    srm_set_ctl(dev_entry);

    /* Get card queue pointers */

    srm_get_pointers(dev_entry);

    /* Enable interrupts */

    srm_regs->interrupt = 0x80;

    return(0);
}

srm_reset_rcv_queues(dev_entry)
    register struct srm_table *dev_entry;
{
    register int i;
    register struct srm_reg *srm_regs;

    srm_regs = dev_entry->srm_regs;

    /* Reset queues -- If we can't get the semaphore */
    /* we just return.                               */

#ifdef SRM_MEASURE
    if ((i = srm_crit(&srm_regs->semaphore,dev_entry)) != -1) {
#else
    if ((i = srm_crit(&srm_regs->semaphore)) != -1) {
#endif

	/* Reset rcv data and ctrl queues */

	*(dev_entry->rx_data_qempty_ptr) =
	    *(dev_entry->rx_data_qfill_ptr);
	*(dev_entry->rx_data_qempty_ptr + 2) =
	    *(dev_entry->rx_data_qfill_ptr + 2);

	*(dev_entry->rx_ctrl_qempty_ptr) =
	    *(dev_entry->rx_ctrl_qfill_ptr);
	*(dev_entry->rx_ctrl_qempty_ptr + 2) =
	    *(dev_entry->rx_ctrl_qfill_ptr + 2);

	/* Release semaphore */

	srm_regs->semaphore = 0;
	UNCRIT(i);
    }

    return;
}
