/*
 * @(#)netdiag0.c: $Revision: 1.7.83.5 $ $Date: 93/12/08 11:42:57 $
 * $Locker:  $
 *
 */

/* This file has been corrected after running through lint 4/26/90 */


#if defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) $Header: netdiag1.c,v 1.7.83.5 93/12/08 11:42:57 marshall Exp $";
#endif

/* BEGIN_IMS	netdiag1.c
****************************************************************************
**** 
****	     netdiag1.c
****
****************************************************************************
* Input Parameters
* Output Parameters
* Return Values
* Globals Referenced
*
* Description
*     NS/OSI diagnostics pseudo driver for Logging and Link Level Tracing.
*
*
* Algorithm
*    That of an ordinary character driver
*
* Externally Callable Routines
* Test Module
* To Do List
* Notes
*  As of initial revision, this file will compile specifically for OSI. NS
*  will have to add ifdef's to get around OSI. Also, for OSI this driver has
*  another flag 'PPT'. This is for the second release of KTL when per path and
*  per dest tracing have been added to Tansit.
*
* Modification History
*   9/10/88  K.Cirimele
*   2/1/89   K.Cirimele - Updates from the code review.
*   2/21/89  K.Cirimele -  Removed the status parameter from the diag_send_msg
*			   and netdiag_recv_msg functions.
*
*   6/11/90  K.Vandiver - Updated from keeping mbufs around to keeping
*			  kernel buffers instead. Better efficiency and easier
*			  for CPE. 
*
*
*****************************************************************************
* END_IMS    netdiag1.c	  */

/* Network Services diagnostics driver */

#ifndef _KERNEL
#define _KERNEL
#endif 

#include "../h/types.h"
#include "../h/ioctl.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/uio.h"		/* iovec struc				*/
#include "../h/systm.h"	      /* to get maximum memory avail	      */
#include "../h/sysmacros.h"
/* #include "../h/cmap.h" */	      /* to get the max # of pages avail      */
#include "../h/errno.h"
#include "../h/file.h"
#include "../h/subsys_id.h"
#ifdef __hp9000s800
#include "../sio/llio.h"
#endif


#ifndef _KERNEL
#include "../h/net_diag.h"
#include "../h/netdiag1.h"
#else  _KERNEL
#include "../h/netdiag1.h"
#include "../h/net_diag.h"
#endif _KERNEL




extern int copyin();
#ifdef DEBUG
int netdiag_log_msg=0;
int netdiag_tr_msg=0;
#endif

extern int net_set_masks();	/* why aren't these routines in this file? */
extern int net_reset_masks();
extern int net_get_status();

/*-------------------  ques for tracing/logging ------------------------ */
/* Tracing and logging each have their own ques. They are indexed by the */
/* device minor number which is 0 for tracing and 1 for logging.	 */
/*---------------------------------------------------------------------- */
que_elements_t netdiag_ques[NO_QUES] = {
     { FALSE,TRUE,0,0,DIAG_CLOSED,0,NETDIAG_Q_DF,0,0,0,FALSE,NULL},
     { FALSE,TRUE,0,0,DIAG_CLOSED,0,NETDIAG_Q_DF,0,0,0,FALSE,NULL},
};


/* index to determine if tracing or logging is on for NS or OSI, */   
/* the entries correspond to the minorvalue */
int netdiag_on[2];

caddr_t netdiag1_malloc();
caddr_t netdiag_recv_msg();



/* BEGIN_IMS netdiag1_open    *
**************************************************************************
**** 
****   netdiag1_open (dev, open_flag) 
**** 
**************************************************************************
* 
* Input Parameters:
*    dev_t  dev;	  contains the major and minor number of this driver
*    int    open_flag;	  contains the open flag bits for the open system call,
* 
* Output Parameters:
* Return values:
*	0	   - no error
*	EBUSY	   - indicates driver is already open or is busy initializing
*	EPERM	   - the user trying to open the driver is not a superuser
*	ENXIO	   - the minor number is illegal
*
* Globals Referenced:
* Description
*   - To open the driver for kernel tracing and logging use. There are 4 
*     different minor numbers allowed to officially open the driver. 
*     Two for NS and two for OSI. Opening with OSI requires that you
*     pass in open flags. These flags keep track on who is openning the
*     driver and what tasks to perform. 
*
* Algorithm:
*   - If not superuser return(EPERM)
*   - If flags = FWRITE, then return(0);
*   - If flags = FWRITE && FAPPEND, check if the driver is opened for 
*	    reading. If open for reading, return (EBUSY); 
*   - if flags = FREAD, allocate a buffer, and set up for receiving 
*	    messages.
*
* To do list
* Notes
* Modification History
*   9/14/88  K.Cirimele	       initial revision
*   2/1/89   K.Cirimele	       Updates from the code review
*	     - Compacted this call by using netdiag_ques[minoral] instead of
*	       the pre-defined alaises for each one. 
*	     - Added a check for the FREAD flag
*
* External Calls
* Called By
*   The tracing and logging facilities / ktl daemon or any superuser process
*
*****************************************************************************
* END_IMS   netdiag_open  */

int netdiag1_open(dev, open_flag)
	dev_t	dev;
	int	open_flag;
{
	
	int	minorval;
	
	minorval = minor(dev);
#ifdef DEBUG
	printf("Opening netdiag1 driver minorval = %d \n", minorval);
#endif
	
	
	/* filter out the bad minor values right away, NS can be added later */
	if ((minorval != NET_TRACE) && 
	    (minorval != NET_LOG))	 
		return(ENXIO);
	
	/* check for superuser returns 0 if not superuser and 1 if superuser	 */
	if (!suser()) {    
		return(EACCES);
	}  
#ifdef DEBUG	 
	printf("Open flag = %d\n", open_flag);
#endif
	
	/*-----------------------------------------------------------------*/
	/* check the open flag, if it is FWRITE this means it is an	      */
	/* 'empty' open, so just return. If the flag is FREAD  it is	 a    */
	/* 'real' open and the process opening will initialize buffers and */
	/*  set a flag indicating the driver was opened by a process that  */
	/*  will do continuous reads.  If the flag is FWRITE | FAPPEND     */
	/*  check to see if the driver is openned for reading already and  */
	/*  if so return EBUSY.					      */
	/*-----------------------------------------------------------------*/
	if (open_flag == FWRITE) { 
		return(0);
	}
	
	if (open_flag == (FWRITE | FAPPEND)) {
		if (netdiag_ques[minorval].que_state == DIAG_OPEN)
			return(EBUSY);
		else 
			return(0);
	}  /* if open_flag == FWRITE && FREAD */
	
	
	
	if (open_flag == FREAD) { 
		switch (minorval) {
		case NET_TRACE:  
		case NET_LOG:
			if (netdiag_ques[minorval].que_state == DIAG_OPEN) { 
				return(EBUSY);
			}
			
			netdiag_ques[minorval].que_state = DIAG_OPEN;
			netdiag1_newbuf(minorval, netdiag_ques[minorval].que_size);
			break; 
			default:  return(ENXIO);
			
		}	/* end of switch on minorval */ 
		
	}  /* if open_flag == FREAD */
	else  {
		return(EINVAL);
	}
	
	return (0);
	
}   /* end of netdiag1_open */



/* BEGIN_IMS netdiag1_close    *
**************************************************************************
**** 
****  int netdiag1_close (dev)
**** 
**************************************************************************
* 
* Input Parameters:
*     dev_t dev	   - contains the major and minor number of the device to
*		     be used.
* Output Parameters:
* Return values:
*	0	   - no error, all close routines return 0
*
* Globals Referenced:
* Description
*  - To close the driver for kernel tracing or logging for a specific 
*    architecture. 
*  - If the driver was closed as a result of a killed daemon, then all
*    masks are saved in the controllers. 
*  - If the driver was closed intentionally, then the filters are reset in
*    the controllers and all subsystem are turned off.
*  - deallocate the buffer ques
* 
* Algorithm:
*    - extract the minor number
*    - switch on the minor number and determine if the driver was close
*      as a result of a dead daemon or not. If the daemon was killed, then
*      the filters are saved, else they are cleared. This is determine
*      by checking the que state of the que for that minor value.
* 
* To Do List
* Notes
* Modification History
*   9/14/88   K.Cirimele    initial revision
*   2/1/89    K.Cirimele    Updates from code review
*	      - added splx around global parameter that are updated
*	      - deallocates the buffer instead of having ioctl do it
*
* External Calls
*   net_stop
*   net_reset_masks
*   netdiag1_newbuf
*   
* Called by(optional)
*
******************************************************************************
* END_IMS netdiag1_close    */


int netdiag1_close(dev)
	dev_t	dev;
{
	
	rs_masks	reset_arg;
	int		minorval;
	int	        i, s;
	
	
	minorval = minor(dev);
	
	s = splimp();
	/* set the driver as closed */
	netdiag_ques[minorval].que_state= DIAG_CLOSED; 
	splx(s);
	
	switch (minorval) {
	case NET_TRACE:	 net_trace_on = 0;
		break;
	case NET_LOG:	 net_log_on = 0;
		break;
	}
	
	/* deallocate the buffer */
	netdiag1_newbuf(minorval,0);
	
	/*-----------------------------------------------*/
	/* determine if the daemon was killed or not by  */
	/* looking at on flag  If it is set,then the	    */ 
	/* daemon was killed. If it is not set, a process*/ 
	/* intended to turn off tracing and close the    */
	/* driver. So its okay to reset the filters.	    */
	/*-----------------------------------------------*/
	
	if (netdiag_on[minorval] == NETDIAG_OFF) {
		/*-----------------------------------------------*/
		/* set up reset argument and make a reset call   */
		/* for each subsystem, this close is intentional */
		/* since this driver sets up the params, it will */
		/* not check the return values.		       */
		/*-----------------------------------------------*/
		
		for (i=0; i< MAX_SUBSYS; i++)  
		{
			reset_arg.subsys_id = i;	
			reset_arg.rs_filters = RS_ALL_NET_TR;
			net_reset_masks(minorval, &reset_arg);
		}
	}
	return (0);
	
}   /* end of netdiag1_close */





/* BEGIN_IMS netdiag1_read   *
**************************************************************************
**** 
****  int netdiag1_read(dev, uio) 
**** 
**************************************************************************
* 
* Input Parameters:
*    dev_t dev	    - contains both the major and minor number of the device
*		      to be used
*    struct uio *uio- contains a pointer to the uio structure which has info 
*		      pertaining the the data transfer
*
* Output Parameters:
* Return values:
*	0	   - no error
*	-1	   - indicates the transfer was unsuccessful
*     EIO	   - tells the reading process(ktl_daemon) to exit 
*		    
* Globals Referenced:
* Description:
*	To transfer tracing and logging packets to the caller. This module
*	will call a buffer manager to get the actual data and make a uiomove
*	call to transfer the data to user space. 
*
* Algorithm:
*    Check if superuser, then test the dev number to deterine what buffer
*    que to read from.	
*    If there are no messages to be read, then set a sleep flag and sleep
*    on the que. Recv_msg will wakeup and give netdiag1_read the message.
*    Next, the message must be copied to user space using uiomove with the
*    mbuf. 
*
* To Do List
* Notes
* Modification History
*   9/14/88  K.Cirimele	   initial revision
*   2/1/89   K.Cirimele	   Updates from code review
*	     - Removed all dependencies on 'status' from the old 
*	       netdiag_recv_msg()
*	       function 
*	     - Added dependencies on the mbufptr instead of a 'status' parameter
*	     - Added a while() loop for checking the msgs from 
*	       netdiag_recv_msg()
*	       and for checking the que_kill flag in netdiag_ques.
*	     - cleaned up putting the dropped packets value into the ktl header
*	       part of the mbuf.
*   6/11/90  - K.Vandiver  
*	     - converted from mbufs to kernel buffers for better efficiency.
*
* External Calls
*   net_user	
*   netdiag_recv_msg
*   sleep
*   FREE
*
* Called By (optional)
*   Superuser processes
*
*****************************************************************************
* END_IMS   netdiag1_read */

int netdiag1_read(dev, uio)
	dev_t	    dev;
	struct uio *uio;
{
	
	int		     	err;
	int		     	minorval;
	int		     	len;
	int		     	priority = PZERO + 1;  /* priority for sleeping on read */
	caddr_t			bufptr;
	struct ktl_buf_t*  	bp_buf;
	ktl_msg_hdr_type   	*tempbuf;
	int		     	s;
	
	
	minorval = minor(dev);
	
	/*-------------------------------------------------------------------*/
	/* Try and read a message. If it comes back QUE_EMPTY, then	     */
	/* check the kill flag for that que. If the kill flag is set, dealloc*/
	/* the que and then return EIO to the reader. ELSE, sleep on the que */
	/* until it is awakened by diag_send_msg, or NETD_STOP.		     */
	/*-------------------------------------------------------------------*/
	
	/* attempt to read a message */
	s=splimp();
	while (( bufptr = (caddr_t) netdiag_recv_msg(minorval)) == NULL) 
	{  
		if (netdiag_ques[minorval].que_kill == 1) 
		{
			splx(s); 
			/* que is dealloced by netdiag1_close */
			return(EIO);
		}
		else 
		{
			netdiag_ques[minorval].que_sleep = TRUE;   
			sleep(&netdiag_ques[minorval].que_msgs_qued, priority);
			/* wake up here */
			netdiag_ques[minorval].que_sleep = FALSE;   
		}
	}  /* while netdiag_recv_msg == NULL */
	splx(s);
	
	/* ----------------------------------------------------------------*/
	/* cast the bufptr to ktl_buf_t so we can get the len of data. Then*/
	/* copy all data to user space with one big uiomove()		   */
	/* ----------------------------------------------------------------*/
	bp_buf = (struct ktl_buf_t*) bufptr;
	tempbuf = (ktl_msg_hdr_type*) &bp_buf->base;
	
	/* the header is there, so length will never be negative */
	len = bp_buf->len;
	
	/* I don't understand why this is here.	 Why not just make the 
	   assignment in ktrc_write and klogg_write??.  I make the assignment there
	   so this is commented out:
	   tempbuf->packetlen = len - sizeof(ktl_msg_hdr_type);
	   Also, I've changed the semantics of the information.  packetlen is the amount
	   of original data that was to be traced or logged.  tracedlen is the amount of 
	   data that is actually traced or logged. Given that, the above statement should 
	   be:
	   tempbuf->tracedlen = len - sizeof(ktl_msg_hdr_type);
	   */
	
	/* 
	 * If dropped_packets is always set to 0
	 * What is this info used for then?
	 * How do we report the que_msgs_dropped data?
	 * 3-Mar-92 TM
	 */
	/* set the msg dropped for the particular subsystem */
	/* tempbuf->dropped_packets = 0; */
	
	
	/* 
	 * Piggyback the msgs dropped info
	 * since this packet is making it out
	 * of the kernel.
	 */
	tempbuf->dropped_packets = netdiag_ques[minorval].que_msgs_dropped;
	s=splimp();
	netdiag_ques[minorval].que_msgs_dropped = 0;
	splx(s);
	
	/*------------------------------------------------------------------*/
	/* copy the packet out to user space via uiomove		      */
	/*------------------------------------------------------------------*/
	err = uiomove ((caddr_t)&bp_buf->base, bp_buf->len, UIO_READ, uio);
#ifdef DEBUG
	printf("Error on uiomove in netdiag1_read\n");
#endif
	/* free the kernel buffer and return */
	FREE(bufptr, M_DYNAMIC);
	return(err);
	
}  /* end of netdiag1_read */


/* BEGIN_IMS netdiag1_ioctl    *
**************************************************************************
**** 
****  int netdiag1_ioctl(dev, command, argp)
****
**************************************************************************
* 
* Input Parameters:
*   dev_t      dev	contains both the major and minor number of the device
*			to be used
*   int	     command	contains the NETDIAG ioctl command to be performed
*   caddr_t  argp	caddr_t, pointer to arguments  for the ioctl command
*  
* Output Parameters:
* Return values:
*	0		- no error
*	EINVAL		- invalid request or parameter
*		    
* Globals Referenced:
* Description
*    To perform ioctl requests from a user space process.  This module 
*    redirects most requests to the specific controller.
*	o    allocates a new buffer
*	o    sets trace masks
*	o    sets log masks
*	o    reset trace and log masks to default
*	o    starts tracing or logging
*	o    stops tracing or logging
*	o    used to retrieve current  filter values
*    
* Algorithm:
*   According to the minor number and ioctl command, either make a call
*   to the OSI or NS controller; or handle the command here. 
*
* To Do List
* Notes
* Modification History
*    9/14/88   K.Cirimele     initial revision
*    2/1/89    K.Cirimele     Updates from the code review
*	       - created a structure netdiag_on[] which has allowed most ioctl
*		 calls to be compacted. 
*	       - since the net_diag.c routine returns EINVAL with a bad subsys
*		 ID value, this routine now returns the call directely.
*
* External Calls
*   copy_in()
*   net_start()
*   net_stop()
*   net_reset_masks()
*   net_set_masks()
*   net_get_status()
*   uiomove()
*   minor()
*
* Called By (optional)
*
******************************************************************************
* END_IMS    netdiag1_ioctl   */

netdiag1_ioctl(dev, command, argp)
	dev_t	  dev;
	int	  command;
	caddr_t	  argp;
{
	
/*	int	     err;	*/
	int	     minorval;
/*	int	     size;	*/
/*	dm_mask_t    set_arg;	*/
/*	dm_mask_t    *stat_p;	*/
/*	rs_masks     reset_arg; */
/*	struct uio   *uio;	*/
	
	
	
	/* extract the minor number */
	minorval = minor(dev);
	
	/* get rid of any minorval != to OSI, but leave hooks in code */
	if ((minorval != NET_TRACE) && (minorval != NET_LOG)) 
	{
		return(EINVAL);
	}
	
	/* switch on the command to determine what should be done */
	switch (command)  {
		
	case  NETD_NEW_BUF :  
		/*-----------------------------------------------------*/
		/* Allocate a buffer for the queues. A buffer can only */
		/* be allocated while t/l is NOT on. Otherwise it can  */
		/* not be alloc'd dynamically.			       */
		/*-----------------------------------------------------*/
#define ls    ((int*)argp)
		
		if (netdiag_on[minorval] == NETDIAG_ON) 
		{
			return (EBUSY);	 /* cannot dynamically chng sz */
		}
		
		/* determine if the size sent in indicates default or 
		   not and if it is out of range */
		
		if ((*ls < 0) || (*ls > NETDIAG_MAX_Q)) 
		{
			return(EINVAL);
		}
		
		/* set the default queue size if one wan't given */
		if (*ls == 0) 
		{  
			*ls = NETDIAG_Q_DF; 
		}
		
		/* Space for the queue was allocated when the device */
		/* was opened.  We must give back that space before  */
		/* taking any more.  There shouldn't be anything in  */
		/* the queue yet since NETDIAG_ON is false.          */
		netdiag1_newbuf(minorval, 0);

		/* make the call to the buffer routines and get back */
		/* the size actually allocated */
		*ls = netdiag1_newbuf(minorval, *ls);
		if (*ls == NULL)
		{ 
			return(ENOSPC);
		}
		
		netdiag_ques[minorval].que_state = DIAG_OPEN;
		return(0);
#undef ls
		break;
		
	case NETD_SET_MASK : 
		/*------------------------------------------------*/
		/* send the request to the specific controller so */
		/* filters may be set, check the minor number and */
		/* parameters to determine the controller.	  */
		/* check the subsystem passed in to see if it is  */
		/* valid.					  */
		/*------------------------------------------------*/
#define ls    ((dm_mask_t*)argp)
		return (net_set_masks(minorval, ls)); 
		break;
#undef	 ls
		
		
		
	case NETD_RESET_MASK :	
		/*------------------------------------------------------------*/
		/* send the request to the specific controller so filters may */
		/* be reset to default values for OSI only the host T/L will  */
		/* reset masks. Check the minor number to determine the	      */
		/* controller. The ktl_daemon will never make a reset masks   */
		/* call, so the minor number is never even checked for.	      */
		/*------------------------------------------------------------*/
#define	 ls  ((rs_masks*)argp)
		return(net_reset_masks(minorval, ls));
#undef	 ls
		
#ifdef KTL_GET_STATUS
	case  NETD_GET_STATUS :	 
		/*------------------------------------------------------*/
		/* send the request to the specific controller		*/
		/* - check the minor number to determine the controller */
		/* - for OSI, only the host T/L will get the status	*/
		/*------------------------------------------------------*/
		
#define	  ls   ((dm_mask_t*)argp)
		return(net_get_status(minorval, ls)); 
#undef	ls
		
		break;	/* end of switch on NETD_GET_STATUS */
#endif KTL_GET_STATUS
		
		
	case NETD_START :  
		/*------------------------------------------------------*/
		/* set a flag indicating tracing or logging has started */
		/* and send the request to the specific controller	*/
		/* Flag the que not to send back EIO if set to kill	*/
		/* - only the ktl daemon will start t/l for OSI		*/
		/*------------------------------------------------------*/
		
		netdiag_on[minorval] = NETDIAG_ON;
		netdiag_ques[minorval].que_kill = 0;
		switch (minorval) {
		case NET_TRACE: 
			net_trace_on = 1;
		case NET_LOG:	
			net_log_on   = 1;
		}
		return(0);
		
	case NETD_STOP	:   
		/*------------------------------------------------------*/
		/* set a flag indicating tracing or logging has stopped */
		/* and send the request to the specific controller	*/
		/* for OSI, only the host T/L will stop tracing		*/
		/* - tell the buffer manager to deallocate the buffer	*/
		/* after the last message has been read out of the que	*/
		/* wake up any proc that is sleeping on the que.	*/
		/*------------------------------------------------------*/
		
		netdiag_ques[minorval].que_kill = 1;
		if (netdiag_on[minorval] == NETDIAG_OFF) {
			return(0);
		}
		netdiag_on[minorval] = NETDIAG_OFF;
		switch (minorval) {
		case NET_TRACE:	 
			net_trace_on = 0;
			break;
		case NET_LOG:	 
			net_log_on = 0;
			break;
		}
		if (netdiag_ques[minorval].que_sleep) {
			wakeup (&netdiag_ques[minorval].que_msgs_qued);
		}
		return(0);;
		
	/* default for the main switch statment for ioctl */
	default: return(EINVAL);
		break;
		
	}  /* end of switch on the commands for	 netdiag1_ioctl */
	
}  /* end of netdiag1_ioctl */



/* BEGIN_IMS netdiag1_write    *
**************************************************************************
****
****	       netdiag1_write()
****
**************************************************************************
*
* Input Parameters
*	dev_t dev	Diagnostics device from which reading 
*	struct uio *uio usual user i/o structure
*
* Output Parameters
*	none
*
* Return Value
*	0		one message successfully written.
*	EACCES		invalid minor number
*	EINVAL		record size too small or too large.
* Globals Referenced
*
* Description
*	Write routine for the diagnostics driver.
*	Used to write messages from User Space to Kernel Driver.
*
*	An log/trace structure is read from user space and sent to
*	the logging/trace subsystem with a call to the appropriate macro.
*
* Algorithm
*    none. 
* Notes:
*
* To Do List:
*
* Modification History 
*   9/1/88   K.Cirimele	   initial revision
*   2/1/89   K.Cirimele	   no updates from code review
* 
* Called by:
*   NS user processes
*
**************************************************************************
* END_IMS netdiag1_write    */

int netdiag1_write(ptr, buf_id)
caddr_t ptr;
int	buf_id;
{
				/* why are the parms so weird? */
}  



/* BEGIN_IMS netdiag_send_msg *
 ********************************************************************
 ****
 ****	netdiag_send_msg(dev_minor,msg_buf )
 ****
 ********************************************************************
 * Input Parameters
 *	dev_minor			Minor number of pseudo dev
 *	msg_buf				The ptr to message to be qued
 *     
 * Output Parameters
 *	0 
 *  
 * Return Value
 *	none
 * Globals Referenced
 *
 * Description
 *	diagnostics message queue - send_msg adds a message to a circular
 *	buffer.	 The queue is LIFO.  When there is no room in the buffer
 *	the oldest message is dropped.	The mbuf(s) that the pointer is
 *	pointing to is freed. 
 *
 *	After the message has been qued then send_msg will do a wakeup
 *	call on the message counter.  This is so that all processes
 *	sleeping on the counter address will wakeup. 
 *	This is going to be used to sync sending and receiving.	 
 *
 *	This procedure will only manage the MBUF Pointers.  It is 
 *	upto the calling procedures to allocate or free MBUF's.
 *	The only exception is when the que is full and a mbuf pointer
 *	needs to be discarded.	Then this procedure will free the mbufs.
 *
 * Algorithm
 * To Do List
 * Notes
 *
 * Modification History
 *	6/30/88	 rohit		 Ported from nsdiag0.c
 *	1/89	 K.Cirimele	 This version has **que_start so mbufs are
 *				 freed correctly.
 *	2/1/89	 K.Cirimele	 Updates from code review
 *		 - Changed all > signs to >= checks
 *		 - moved decrement of msgs_qued to netdiag_recv_msg()
 *		 - do a check of the tail value against que_msgs_qued
 *		   instead of comparing it with head.
 *		 - removed unnecessary checks that this routine used to
 *		   do
 * External Calls
 *
 * Called By (optional)
 *
 *  ktrc_write()
 *  klogg_write()
 *
 ********************************************************************
 * END_IMS netdiag_send_msg */

netdiag_send_msg(dev_minor,msg_buf)
	int	      dev_minor;
	caddr_t	      msg_buf;
{
	int	      s;
	caddr_t	      temp;
/*	caddr_t	      tempmbuf; */
/*	int	      status;	*/
	struct iovec* test_iovec;     /* for debug only */
	que_elements_t	*cur_q = &netdiag_ques[dev_minor]; /*debugging*/
	
#ifdef DEBUG 
	/* count the thruput number of messages */
	if (dev_minor == NET_TRACE)
		netdiag_tr_msg++;
	else
		netdiag_log_msg++;
#endif 
	
	
	s = splimp();	

	/* que may not have been allocated */
	if (!netdiag_ques[dev_minor].que_start) {
		splx(s);
		FREE(msg_buf,M_DYNAMIC);
		return;
	}
	
	/*
	 * if we are at the high water mark, dequeue the message
	 */
	
	if (netdiag_ques[dev_minor].que_msgs_qued >= 
	     netdiag_ques[dev_minor].que_high_water_mark){
		/* We have to drop a message. */
		temp = netdiag_recv_msg(dev_minor );	
		if (!temp) {
			splx(s);
			FREE(msg_buf,M_DYNAMIC);
			return;
		}
		
		/* free any memory associated with the mbuf ptr */
		/* temp is a pointer to a mbuf			*/
		FREE(temp,M_DYNAMIC);
		
		netdiag_ques[dev_minor].que_msgs_dropped++;
	}
	
	netdiag_ques[dev_minor].que_start[netdiag_ques[dev_minor].que_tail++]
		= msg_buf;
	
	netdiag_ques[dev_minor].que_msgs_qued++;	/* count the message */
	
	if (netdiag_ques[dev_minor].que_tail >= 
	    netdiag_ques[dev_minor].que_high_water_mark) 
		netdiag_ques[dev_minor].que_tail = 0;
	
	
	splx(s);
	
	/* Wakeup any processes sleeping on this address, first */
	/* check the sleep flag for the que to see if anyone is */
	/* sleeping.						*/
	if (netdiag_ques[dev_minor].que_sleep) {
		wakeup(&netdiag_ques[dev_minor].que_msgs_qued);
	}
	
	return;
}		/***** send_msg *****/




/* BEGIN_IMS netdiag_recv_msg *
 ********************************************************************
 ****
 ****	netdiag_recv_msg(dev_minor )
 ****
 ********************************************************************
 * Input Parameters
 *	dev_minor			Minor number of device
 *
 * Output Parameters
 *	msg_buf				The caddr_t buffer
 *
 * Return Value
 *	none
 * Globals Referenced
 *
 * Description
 *	diagnostics message queue -  recv_msg removes one msg form que.
 *	This procedure is used to deque mbuf pointers from the que.
 *	It is up to the calling procedure to free the MBUF's.  
 *
 * Algorithm
 * To Do List
 * Notes
 *
 * Modification History
 *	6/30/88	 rohit		 Ported from nsdiag0.c
 *	1/89	 K.Cirimele	 This version has que_start as **que_start
 *				 so the mbufs are freed correctly.
 *	2/1/89	 K.Cirimele	 Updates from code review
 *		 - added que_msgs_qued decrement here instead of in diag_send_
 *		   msg. 
 *		 - check que_msgs_qued instead of the que_empty and que_full
 *		   fields 
 *		 - removed unnecessary checks that this routine used to do. 
 *	6/11/90	 - k.vandiver changed from mbufs to kernel buffers
 * External Calls
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS netdiag_recv_msg */

caddr_t netdiag_recv_msg(dev_minor )
	int		     dev_minor;
{
	int		   s;
	caddr_t		   msg_ptr;
	int		   head_ptr;
	struct iovec*	   test_iovec;	  /* iovec for DDB testing */
	que_elements_t	*cur_q = &netdiag_ques[dev_minor];/* debugging */ 
	
#ifdef DEBUG
	printf("Entering diag_RECV_msg with msg_qued = %d\n",
	       netdiag_ques[dev_minor].que_msgs_qued );
#endif
	
	s = splimp();
	
	/* check is there is a que  */
	if (!netdiag_ques[dev_minor].que_start) {
		splx(s);
		return((caddr_t) 0);
	}
	
	/* if there are zero msgs in que then return */
	if (netdiag_ques[dev_minor].que_msgs_qued == 0) {
		msg_ptr = 0;
	} else {
		head_ptr = netdiag_ques[dev_minor].que_head;
		
		msg_ptr = netdiag_ques[dev_minor].que_start[head_ptr];
		netdiag_ques[dev_minor].que_msgs_qued--;
		netdiag_ques[dev_minor].que_head++;
		
		if (netdiag_ques[dev_minor].que_head >= 
		    netdiag_ques[dev_minor].que_high_water_mark)
			netdiag_ques[dev_minor].que_head = 0;
		
	}
	splx(s);
	return(msg_ptr);
}		/***** recv_msg *****/



/* BEGIN_IMS netdiag1_newbuf *
 ********************************************************************
 ****
 ****		netdiag1_newbuf(dev_minor,que_size)	
 ****
 ********************************************************************
 * Input Parameters
 *   dev_minor	minor number of device
 *   que_size	size of que in number of Mbuf pointers
 *
 * Output Parameters
 * Return Value
 *   que_size:	size of que actually  allocated.
 *
 * Globals Referenced
 * Description
 *  Given a que size, in number of message pointers to be qued,
 *  this routine tries to allocate system memory for the que.
 *  If the requested que size is so large that memory cannot be allocated
 *  because system memory is not available then the default que size will
 *  be allocated.  Care is taken not to allocate too much system memory
 *  for logging and tracing.  This is so that logging and tracing will
 *  have minimal impact on system performance.
 *
 *  if que_size is zero then we deallocate all the memory.  It is up to
 *  the calling procedure to make sure all the mbufs were freed before
 *  this procedure is called with a zero que size.
 *
 * Algorithm
 * To Do List
 * Notes
 * Modification History
 *	7/13/88 Rohit	  ported from nsdiag0
 *	2/1/89	K.Cirimele  no changes from code review
 *	6/11/90 K.Vandiver  changed from mbufs to caddr_t's for storage
 *
 * External Calls
 * Called By (optional)
 ********************************************************************
 * END_IMS netdiag1_newbuf */

int netdiag1_newbuf(dev_minor,que_size) 
	int	dev_minor;	      /* minor number		   */
	int	que_size;	      /* size in number of msgs */
{
	int	newsize = que_size;
	int	i;
	caddr_t newqueue;
	caddr_t oldqueue;

	i = splnet();
	oldqueue = netdiag_ques[dev_minor].que_start;
	netdiag_ques[dev_minor].que_start = 0;

	newqueue = (caddr_t)netdiag1_malloc(dev_minor,&newsize,oldqueue);

	netdiag_ques[dev_minor].que_start	 = (char **)newqueue;
	netdiag_ques[dev_minor].que_msgs_qued	 = 0;
	netdiag_ques[dev_minor].que_full	 = QUE_EMPTY;
	netdiag_ques[dev_minor].que_empty	 = QUE_EMPTY;
	netdiag_ques[dev_minor].que_head	 = 0;
	netdiag_ques[dev_minor].que_tail	 = 0;
	netdiag_ques[dev_minor].que_msgs_dropped = 0;
	netdiag_ques[dev_minor].que_kill	 = 0;
	netdiag_ques[dev_minor].que_high_water_mark = newsize;

	splx(i);
	
	que_size = newsize;
	return(que_size);
}   /* end of netdiag1_newbuf */



/* BEGIN_IMS netdiag1_malloc *
 ********************************************************************
 ****
 ****		netdiag1_malloc(dev,&que_size,old_ptr)	
 ****
 ********************************************************************
 * Input Parameters
 *  dev	       minor dev number
 *  que_size   Number of mbuf pointers
 *  old_ptr    pointer to where data is stored
 * 
 * Output Parameters
 *  que_size  sends back how much has been allocated
 *     
 * Return Value
 *  pointer of allocated space, can be zero if user sent in que_size of 0.
 *
 * Globals Referenced
 * Description
 *
 *   The amount of memory required to que the requested amount of mbufs
 *   will be checked against a maximum.	 If the requested amount of memory
 *   is too large then the default que size will be used.  If the que size
 *   is zero the allocated memroy will be deallocated.	It is important
 *   that the calling procedure deallocates all the mbufs before calling
 *   this routine with a zero que size.
 *    
 *   The data structure stores pointers to mbufs (this is how the messages are
 *   received in ktrc_write() and klogg_write(). We then try and allocate
 *   a bunch of mbuf pointers and stick them at netdiag_que[].que_start .
 *
 *	  netdiag_que structure
 *	 --------------------	
 *	 | que_full	     |
 *	 | que_empty	     |
 *	 | que_head	     |
 *	 | que_tail	     |
 *	 | que_state	     |
 *	 | que_msgs_qued     |
 *	 | que_size	     |
 *	 | q_high_water_mark |
 *	 | q_msgs_dropped    |
 *	 | que_kill	     |
 *	 | que_sleep	     |
 *	 | que_start	     |	  <<<< this is a caddr_t * struct: que_start
 *	 |		     |
 *	 |		     |----	 struct caddr_t
 *	 --------------------	 |	  ----
 *				 |----->>>|1 |---------------->>> kernel buf
 *					  ----
 *					  |2 |----------------->> kernel buf
 *					  ----
 *					  /  /
 *					  ----
 *					  |n |----------------->> kernel buf
 *					  ----		       ^^^^ bufs come
 *							       from kt/l_write
 *					 ^^^^^ we allocate these pointers
 *
 *   kernel pointers will be qued in the allocated memory, until they are
 *   read out by the trace and log daemons. The first 2 bytes of each kernel
 *   buffer contain the size of that buffer. I therefore jump over it in the
 *   netdiag1_read() routine by casting the buffer to ktl_buf_t. 
 *
 * Algorithm
 *   Memory Allocation Scheme - use of FREE and MALLOC
 *
 * To Do List
 * Notes
 * Modification History
 *	7/13/88 Rohit	 Ported from nsdiag0.c
 *	2/1/89	K.Cirimele no changes from code review
 *	6/1/90	K.Vandiver ^^ same person.
 *		- removed need to store allocated space since BSD 4.3 does
 *		  that for us. 
 *		- no longer look to see if we are grabbing too much memory
 *		  because all I grab are pointers which are 4 bytes each.
 *		- Use the BSD macros FREE and MALLOC to help clean up code and
 *		  make it more readable.
 *
 * External Calls
 *	MALLOC()
 *	FREE()
 * Called By (optional)
 *	netdiag1_newbuf
 ********************************************************************
 * END_IMS netdiag1_malloc */

caddr_t netdiag1_malloc(dev_minor,que_size,oldptr)
	int	  dev_minor;	  /* minor number of dev_minor		  */
	int	  *que_size;	  /* number of msgs, returns actual count */
	caddr_t oldptr;
{
	int newsize;
	
	/*  Check if user wants to deallocate all the memory. (que_size == 0) */
	if (!(*que_size)) { 
		if (oldptr)
			FREE (oldptr, M_DYNAMIC);
		return((caddr_t) NULL);
	}
	
	/* make sure they don't go over the limit */
	if (*que_size >= NETDIAG_MAX_Q) 
		*que_size = NETDIAG_MAX_Q;
	
	newsize = *que_size * (sizeof (caddr_t));
	/*
	 *	Changed MALLOC call to kmalloc to save space. When
	 *	MALLOC is called with a variable size, the text is
	 *	large. When size is a constant, text is smaller due to
	 *	optimization by the compiler. (RPC, 11/11/93)
	 */
	oldptr = (caddr_t) kmalloc(newsize , M_DYNAMIC, M_WAITOK);
	
	return(oldptr);
}



