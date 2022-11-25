/* HPUX_ID: @(#)dillib.c	1.24     86/04/17  */
/*
 **********************************************************
 *        (c) Copyright 1983 Hewlett Packard Co.	  *
 *		   ALL RIGHTS RESERVED			  *
 **********************************************************
 *
 */

#include <sys/param.h>
#include <sys/dil.h>
#include <sys/dilio.h>
#include <sys/gpio.h>
#include <sys/hpib.h>
#include <sys/timeout.h>
#include <dvio.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WSIO
#include <sys/sysmacros.h>
#else
#define _WSIO
#include <sys/sysmacros.h>
#undef _WSIO
#endif  /*_WSIO */

#include <sys/inode.h>
#include <signal.h>
#include <unistd.h>

#define INTERNAL_HPIB	1
#define HP98622		17
#define HP98624		2
#define HP98625		4

#define LOWER_2BITS_MSK  0x03
#define PPOLL_SET	0X10	/* mask to get bit 4 (set or clear ppoll resp)*/
#define SET		1
#define CLEAR		0
#define error_value	-1	/* standard unix error occured value */
#define NUMFILE	1024

extern int errno;

/* memory mapped stuff */
int	map_on = 0;	/* tells the other routines if we are mapping */

struct fd_info	*open_fds[NUMFILE]; /* keep track of open files descripters */
struct fd_info	dil_fds[NUMFILE]; /* keep all open files that are now mapped */
struct fd_info	*dil_fdp[NUMFILE]; /* keep all open files that are now mapped */

struct sc_info isc_info[32];

#define PARITY_CTL	0x08

char libdvio_patch[] = "@(#)PATCH_8.0X	libdvio.a	66.4	91/11/05";



dil_dup_fd(oldfd, newfd) 
{
	dil_fds[newfd] = dil_fds[oldfd];
	open_fds[newfd] = &dil_fds[newfd];
	if (dil_fdp[oldfd])
		dil_fdp[newfd] = &dil_fds[newfd];
}


/*********************************************************************
 *								     *
 *	HPIB_IO library routine					     *
 *	  This will be moved into the kernel later.		     *
 *								     *
 *********************************************************************/
hpib_io (fd, iovec, iolen) 
int fd;     /* file descriptor */
struct iodetail *iovec;	/* vector of vectors */
register int iolen;	/* count of vectors */
{
	register char *buffer;
	register int length;
	register char mode;
	register char pattern;
	register int charsread;
	struct ioctl_type arg;
	struct iodetail *this_iovec;

	/* is the file descriptor reasonable? */
	if ((fd < 0) || (fd >= NUMFILE)) {
		errno = EBADF;
		return -1;
	}

	arg.sc_state = 0;

	/* if io_burst is off use the system call */
	if (!((map_on) && (dil_fdp[fd])))
		return hpibio(fd, iovec, iolen);

	if ((dil_fds[fd].ba & 0x1f) != 0x1f) {
		errno = ENOTTY;
		return(-1);
	}

	while(iolen--) {	/* transfer all the vectors */
		mode = iovec->mode;
		length = iovec->count;
		buffer = iovec->buf;
		pattern = iovec->terminator;
		this_iovec = iovec;
		iovec++;	/* go to next vector */

		if (mode & HPIBREAD) {	/* read transfer */
			/* turn on eol control */
			if (mode & HPIBCHAR) {
				if (io_eol_ctl(fd, 1, pattern) == -1)
					goto error_exit;
			} else
				if (io_eol_ctl(fd, 0, pattern) == -1)
					goto error_exit;

			/* do the transfer */
			if ((charsread = read(fd, buffer, length)) == -1)
					goto error_exit;
			this_iovec->count = charsread;

			/* turn off eol control */
			if (mode & HPIBCHAR)
				if (io_eol_ctl(fd, 0, pattern) == -1)
					goto error_exit;
		}
		else {	/* write transfer */
			if (mode & HPIBATN) {
				if (hpib_send_cmnd(fd, buffer, length) == -1)
					goto error_exit;
			} else {
				if (mode & HPIBEOI) {
					if (hpib_eoi_ctl(fd, 1) == -1)
						goto error_exit;
				} else
					if (hpib_eoi_ctl(fd, 0) == -1)
						goto error_exit;

				/* do the transfer */
				if (write(fd, buffer, length) == -1)
						goto error_exit;

				if (mode & HPIBEOI)
					if (hpib_eoi_ctl(fd, 0) == -1)
						goto error_exit;
			}
		}
	}
	return(0);

error_exit:
	(--iovec)->count = -1;
	return -1;
}

/*********************************************************************
 *								     *
 *	HPIB library routines					     *
 *								     *
 *********************************************************************/
hpib_send_cmnd (fd, ca, length)  /* Send data down HP-IB with ATN asserted */
int fd;     /* file descriptor */
char *ca;   /* command array */
int length; /* number of bytes in the command array to be written */
{
	struct ioctl_cmd_type arg;	/* to pass to drivers */
	register int i = length;
	register unsigned char *b;
	register int ret_val;

	if (length < 1)
		return(0);

	/* is the file descriptor reasonable? */
	if ((fd < 0) || (fd >= NUMFILE)) {
		errno = EBADF;
		return -1;
	}

	if ((map_on) && (dil_fdp[fd])) {
		if ((dil_fds[fd].ba & 0x1f) != 0x1f) {
			errno = ENOTTY;
			return(-1);
		}
		ret_val = hpib_send_map(dil_fdp[fd], ca, length);
		if (ret_val < 0)
			errno = EIO;
		return(ret_val);
	} else {
		b = arg.data;
		arg.length = length;
		arg.sc_state = 0;
		/* copy the data into the ioctl structure */
		for (i = 0; i < length; i++)
			*b++ = *ca++;

		return(ioctl(fd, HPIB_SEND_CMD, &arg));
	}
}


hpib_bus_status (fd, status_number)  /* return bus status information */
int fd;	      	    /* file descriptor */
int status_number;  /* value to determine which status info will be returned */
{
	int error = 0;
	struct ioctl_type arg;	/* to pass to drivers */
      
	arg.sc_state = 0;
	arg.type = HPIB_BUS_STATUS;
	switch(status_number){
	      case CURRENT_BUS_ADDRESS :  /* return current interface address */
			arg.type = HPIB_ADDRESS;
			if ((error = ioctl(fd, HPIB_STATUS, &arg)) ==
							error_value)
				return error;
			else
				return(arg.data[0]);
			break;
	      case ACT_CONT_STATUS :  /* return 1 if interface is active */
			arg.data[0] = STATE_ACTIVE_CTLR;
			break;
	      case SYS_CONT_STATUS : /* return 1 if interface is active */
			arg.data[0] = STATE_SYSTEM_CTLR;
			break;
	      case TALKER_STATUS : 	/*return 1 if interface is addressed*/
			arg.data[0] = STATE_TALK;
			break;
	      case LISTENER_STATUS:	/* return 1 if interface is addressed*/ 
			arg.data[0] = STATE_LISTEN;
			break;
	      case REMOTE_STATUS : 	/* return a 1 if interface is in    */
			arg.data[0] = STATE_REN;
			break;
	      case SRQ_STATUS:	/* return a 1 if SRQ is set, 0 otherwise */
			arg.data[0] = STATE_SRQ;
			break;
	      case NDAC_STATUS :  /* return a 1 if NDAC is set, 0 otherwise */
			arg.data[0] = STATE_NDAC;
			break;
		default :	         /* this handles the case for an */
			errno = EINVAL;  /* invalid request type (error) */
			error = error_value;
			break;
	}
	if ((error == error_value) ||
		((error = ioctl(fd, HPIB_STATUS, &arg)) == error_value))
		return error;
	else
		return((arg.data[0] ? SET : CLEAR));
}


hpib_ren_ctl (fd, flag)  /* Assert/Deassert REN line */
int fd;		/* file descriptor */
int flag; 	/* line control switch */
{
	int error = 0;
	struct ioctl_type arg;	/* to pass to drivers */
      
  /*
   * determine if user wants to set or clear the REN line 
   */
	arg.sc_state = 0;
	arg.type = HPIB_REN;
	arg.data[0] = (flag ? SET : CLEAR);
	return(ioctl(fd, HPIB_CONTROL, &arg)); /* this sets/clears REN */
}


hpib_eoi_ctl (fd, flag)   /* Enable/disable EOI mode */
int fd;     /* file descriptor */
int flag;   /* switch used as non zero - set EOI mode, 0 clear EOI mode */
{
	struct ioctl_type arg;	/* to pass to drivers */
      
	/* is the file descriptor reasonable? */
	if ((fd < 0) || (fd >= NUMFILE)) {
		errno = EBADF;
		return -1;
	}

	if ((map_on) && (dil_fdp[fd])) {
		if (flag)
			dil_fds[fd].state |= EOI_CONTROL;
		else
			dil_fds[fd].state &= ~EOI_CONTROL;
		return(0);
	}

	arg.sc_state = 0;
	arg.type = HPIB_EOI;
	arg.data[0] = (flag ? SET : CLEAR);
	return(ioctl(fd, HPIB_CONTROL, &arg));
}

hpib_ppoll (fd)  /* Conduct a parallel poll and return integer response byte */
int fd;     /* file descriptor */
{
	struct ioctl_type arg;	/* to pass to drivers */
	int error = 0;

	arg.sc_state = 0;
	arg.type = HPIB_PPOLL;
	error = ioctl(fd, HPIB_STATUS ,&arg);
	return((error == error_value) ? error_value : arg.data[0]);
}

hpib_ppoll_resp_ctl (fd,flag)
int fd;     /* file descriptor */
int flag;   /* used to enable (non zero), disable (0) */
{
	struct ioctl_type arg;	/* to pass to drivers */

	arg.sc_state = 0;
	arg.type = HPIB_PPOLL_IST;
	arg.data[0] = flag; /* parameter for ioctl*/
	return(ioctl(fd, HPIB_CONTROL, &arg));
}

/*
 * The following function waits until ((response byte XOR sense) AND mask)
 * is non zero befor returning. (ie. Wait for a specific response )
 */
hpib_wait_on_ppoll(fd, mask, sense) 
int fd,mask,sense;
{
	struct ioctl_type arg;	/* to pass to drivers */
	int error = 0;
      
	arg.sc_state = 0;
	arg.type = HPIB_WAIT_ON_PPOLL;
	arg.data[1] = mask & 0xff;	/* upper bits do not apply here */
	arg.data[2] = sense & 0xff;	/* upper bits do not apply here */
	error = ioctl(fd, HPIB_STATUS, &arg);
	return((error == error_value) ? error_value : arg.data[0]);
}

hpib_status_wait(fd,status) /* Wait for a specific condition to occur */
int fd;	    /* file descriptor */
int status; /* integer parameter indicating what to wait for (see bus status)*/
{
	int error = 0;
	struct ioctl_type arg;	/* to pass to drivers */
      
	arg.sc_state = 0;
	arg.type = HPIB_WAIT_ON_STATUS;
	switch(status){
		case ACT_CONT_STATUS :    /* return 1 if interface is active */
			arg.data[0] = STATE_ACTIVE_CTLR;
			break;
		case TALKER_STATUS : 	/* return 1 if interface is addressed*/
			arg.data[0] = STATE_TALK;
			break;
		case LISTENER_STATUS:	/* return 1 if interface is addressed*/ 
			arg.data[0] = STATE_LISTEN;
			break;
		case SRQ_STATUS:    /* return a 1 if SRQ is set, 0 otherwise */
			arg.data[0] = STATE_SRQ;
			break;
		default :	         /* this handles the case for an */
			errno = EINVAL;  /* invalid request type (error) */
			error = error_value;
			break;
	}
	if ((error == error_value) ||
		((error = ioctl(fd, HPIB_STATUS, &arg)) == error_value))
		return error;
	else
		return 0;
	/*	return arg.data[0]; */
}

 /* Enable a device to respond to a parallel poll */
hpib_card_ppoll_resp (fd,flag)
int fd;     /* file descriptor */
int flag;   /* used to enable (non zero), disable (0) ppoll response */
{
	struct ioctl_type arg;	/* to pass to drivers */

	arg.sc_state = 0;
	arg.type = HPIB_PPOLL_RESP;
	arg.data[0] = (PPOLL_SET & flag? CLEAR : SET); /* parameters for ioctl*/
	arg.data[1] = flag & 0x07;		/* bus address */
	arg.data[2] = (flag & 0x08) >> 3;		/* sense bit */
	return(ioctl(fd, HPIB_CONTROL, &arg));
}


hpib_rqst_srvce (fd, cv)  /* Set serial poll response byte and SRQ line */
int fd;     /* file descriptor */
int cv;     /* integer control value to set serial poll response byte */
{
	struct ioctl_type arg;	/* to pass to drivers */

	arg.sc_state = 0;
	arg.type = HPIB_SRQ;
	arg.data[0] = cv;
	return(ioctl(fd, HPIB_CONTROL, &arg));
	
}

hpib_spoll (fd, ba)   /* Conduct a serial poll */
int fd;     /* file descriptor */
int ba;	    /* bus address of device to be serially polled */
{
	int error = 0;
	struct ioctl_type arg;	/* to pass to drivers */

	arg.sc_state = 0;
	arg.type = HPIB_SPOLL;
	arg.data[1] = ba;

	if ((error = ioctl(fd, HPIB_STATUS, &arg)) == error_value)
		return error;
	else
		return arg.data[0];
}

hpib_abort (fd)  /* Abort all bus activity and return ctl to system controler */
int fd;     /* file descriptor */
{
	struct ioctl_type arg;	/* to pass to drivers */

	arg.sc_state = 0;
	arg.type = HPIB_RESET;
	arg.data[0] = BUS_CLR;

	return(ioctl(fd, HPIB_CONTROL, &arg));
}

hpib_pass_ctl (fd, ba) /* Pass active control to another bus device */
int fd;     /* file descriptor */
int ba;     /* bus address of device to be new active controller */
{
	int error = 0;
	struct ioctl_type arg;	/* to pass to drivers */

	arg.sc_state = 0;
	arg.type = HPIB_PASS_CONTROL;
	arg.data[0] = ba;
	return(ioctl(fd, HPIB_CONTROL, &arg));
}



hpib_address_ctl (fd, ba)
int fd;		/* file descriptor */
int ba; 	/* bus address */
{
	struct ioctl_type arg;	/* to pass to drivers */
      
	arg.sc_state = 0;
	arg.type = HPIB_SET_ADDR;
	arg.data[0] = ba;
	return(ioctl(fd, HPIB_CONTROL, &arg));
}


hpib_atn_ctl (fd, flag)  /* Assert/Deassert ATN line */
int fd;		/* file descriptor */
int flag; 	/* line control switch */
{
	struct ioctl_type arg;	/* to pass to drivers */
  	/*
   	** determine if user wants to set or clear the ATN line 
   	*/
	arg.sc_state = 0;
	arg.type = HPIB_ATN;
	arg.data[0] = (flag ? SET : CLEAR);
	return(ioctl(fd, HPIB_CONTROL, &arg)); /* this sets/clears ATN */
}


hpib_parity_ctl (fd, flag)
int fd;		/* file descriptor */
int flag; 	/* line control switch */
{
	struct ioctl_type arg;	/* to pass to drivers */

	/* is the file descriptor reasonable? */
	if ((fd < 0) || (fd >= NUMFILE)) {
		errno = EBADF;
		return -1;
	}

	if ((map_on) && (dil_fdp[fd])) {
		if (flag) {
			dil_fds[fd].state |= PARITY_CTL;
		} else
			dil_fds[fd].state &= ~PARITY_CTL;
		return(0);
	}

	arg.sc_state = 0;
	arg.type = HPIB_PARITY;
	arg.data[0] = (flag ? SET : CLEAR);
	return(ioctl(fd, HPIB_CONTROL, &arg));
}


/*********************************************************************
 *								     *
 *	GPIO library routines					     *
 *								     *
 *********************************************************************/
gpio_set_ctl (fd, value)  /* allows setting of the cards control register */
int fd;     /* file descriptor */
int value;  /* value to be written to control register */
{
	struct ioctl_type arg;
	int error = 0;

	/*get new ctl bit setting*/
	arg.sc_state = 0;
	arg.type = HPIB_SRQ;
	arg.data[0] = value;
	return(ioctl(fd, HPIB_CONTROL, &arg));
}


gpio_get_status (fd)  /* return the value of the cards' status register */
int fd;     /* file descriptor */
{
	struct ioctl_type arg;
	int error = 0;

	arg.sc_state = 0;
	arg.type = HPIB_BUS_STATUS;
	error = ioctl(fd, HPIB_STATUS, &arg);
	return ((error == error_value) ? error_value : ( arg.data[0] & 
		LOWER_2BITS_MSK));
}

gpio_eir_ctl(fd, control)  /* don't terminate transfers on eir */
int fd;     /* file descriptor */
int control;
{
	struct ioctl_type arg;

	arg.sc_state = 0;
	arg.type = GPIO_EIR_CONTROL;
	arg.data[0] = control;
	return(ioctl(fd, GPIO_CONTROL, &arg));
}

/*********************************************************************
 *								     *
 *	DIL library routines					     *
 *								     *
 *********************************************************************/

io_dma_ctl (fd, control) 
int fd;     /* file descriptor */
int control;
{
	struct ioctl_type arg;

	arg.sc_state = 0;
	arg.type = IO_DMA;
	arg.data[0] = control;
	return(ioctl(fd, IO_CONTROL, &arg));
}

io_width_ctl(fd, width)  /* select 8 or 16 bit parallel transfers */
int fd;    /* file descriptor */
int width; /* number of bits in width of data path */
{
	struct ioctl_type arg;	/* to pass to drivers */

	/* is the file descriptor reasonable? */
	if ((fd < 0) || (fd >= NUMFILE)) {
		errno = EBADF;
		return -1;
	}

	if ((map_on) && (dil_fdp[fd])) {
		if ((width == 16) && (dil_fds[fd].card_type == HP98622)) {
			dil_fds[fd].state |= D_16_BIT;
			return 0;
		} 
		if (width == 8) {
			dil_fds[fd].state &= ~D_16_BIT;
			return 0;
		} 
		errno = EINVAL;
		return -1;
	}

	arg.sc_state = 0;
	arg.type = IO_WIDTH;
	arg.data[0] = width;
	return(ioctl(fd, IO_CONTROL, &arg));
}


io_timeout_ctl (fd, time)  /* set time limit for I/O operation to complete */
int time;	/* time limit in microseconds to wait until completion */
{
	struct ioctl_type arg;	/* to pass to drivers */

	arg.sc_state = 0;
	arg.type = IO_TIMEOUT;
	arg.data[0] = time;
	arg.data[1] = 0;	/* for series 200 */
	return(ioctl(fd, IO_CONTROL, &arg));
}

io_burst_address (fd, address)  /* set address to use for burst's shmem segment */
int address;	
{
	struct ioctl_type arg;	/* to pass to drivers */

	arg.sc_state = 0;
	arg.type = IO_SET_MAP_ADDR;
	arg.data[0] = address;
	arg.data[1] = 0;	/* for series 200 */
	return(ioctl(fd, IO_CONTROL, &arg));
}

/* the dillib entry point for register access */
io_register_access(fd, data, access_mode, offset)
int *data;
{
	struct ioctl_type arg;
	int error;

	arg.sc_state = 0;
	arg.data[0] = access_mode;
	arg.data[1] = offset;
	if ((access_mode & READ_ACCESS) == 0)
		arg.data[2] = *data;

	if ((error = ioctl(fd, IO_REG_ACCESS, &arg)) < 0)
		return(error);

	if (access_mode & READ_ACCESS)
		*data = arg.data[0];

	return(0);
}

io_reset (fd)  /* reset the interface and initiate self test */
int fd;		/* file descriptor */
{
	struct ioctl_type arg;	/* to pass to drivers */
	int err;

	arg.sc_state = 0;
	arg.type = IO_RESET;
	arg.data[0] = HW_CLR;

	/* is the file descriptor reasonable? */
	if ((fd < 0) || (fd >= NUMFILE)) {
		errno = EBADF;
		return -1;
	}

	err = ioctl(fd, IO_CONTROL, &arg);
	if ((err == 0) && (map_on) && (dil_fdp[fd])) {
		dil_fds[fd].temp = 0; /* we are not in holdoff */
	}
	return(err);
}
  /* enable/disable character match read termination */
io_eol_ctl (fd, flag, match)
int fd;		/* file descriptor */
int flag;
int match;
{
	struct ioctl_type arg;	/* to pass to drivers */

	/* is the file descriptor reasonable? */
	if ((fd < 0) || (fd >= NUMFILE)) {
		errno = EBADF;
		return -1;
	}

	if ((map_on) && (dil_fdp[fd])) {
		if (flag) {
			dil_fds[fd].state |= READ_PATTERN;
			dil_fds[fd].pattern = match;
		} else
			dil_fds[fd].state &= ~READ_PATTERN;
		return(0);
	}

	arg.sc_state = 0;
	arg.type = IO_READ_PATTERN;
	arg.data[0] = flag;
	arg.data[1] = match;

	return(ioctl(fd, IO_CONTROL, &arg));
}

io_get_term_reason (fd)  /*return termination reason for the last read */
int fd;		  /* file descriptor */
{
	struct ioctl_type arg;	/* to pass to drivers */

	/* is the file descriptor reasonable? */
	if ((fd < 0) || (fd >= NUMFILE)) {
		errno = EBADF;
		return -1;
	}

	if ((map_on) && (dil_fdp[fd]))
		return(dil_fds[fd].reason);

	arg.sc_state = 0;
	arg.type = IO_TERM_REASON;
	if (ioctl(fd, IO_STATUS, &arg) == error_value)
		return error_value;
	else
		return arg.data[0];
}

io_speed_ctl (fd, speed)
int fd;		/* file descriptor */
int speed;
{
	struct ioctl_type arg;	/* to pass to drivers */

	arg.sc_state = 0;
	arg.type = IO_SPEED;
	arg.data[0] = speed;

	return(ioctl(fd, IO_CONTROL, &arg));
}

io_lock (fd)
int fd;		/* file descriptor */
{
	struct ioctl_type arg;	/* to pass to drivers */
	int error = 0;

	arg.sc_state = 0;
	arg.type = IO_LOCK;
	arg.data[0] = LOCK;

	error = ioctl(fd, IO_CONTROL, &arg);
	return((error == error_value) ? error_value : arg.data[0]);
}


io_unlock (fd)
int fd;		/* file descriptor */
{
	struct ioctl_type arg;	/* to pass to drivers */
	int error = 0;

	arg.sc_state = 0;
	arg.type = IO_LOCK;

	/*
	 * (In <sys/dil.h>, "UNLOCK" was changed to "DIL_UNLOCK"
	 *  to resolve a conflict with <sys/lock.h>.)
	 */
	arg.data[0] = DIL_UNLOCK;

	error = ioctl(fd, IO_CONTROL, &arg);
	return((error == error_value) ? error_value : arg.data[0]);
}


/************************************************************************
*									*
*	DIL memory mapped library					*
*									*
*	Steps:								*
*		1) IOLOCK(fd)	- see if we can lock the card		*
*		2) ioctl(fd, DIL_MAP, &buf)				*
*			map the card and put important information	*
*			into the buffer(card address, state, pattern... *
*		3) Turn map_on flag on upon completion so the entire	*
*		   Library code knows it may be working with a mapped	*
* 		   I/O card.						*
*									*
************************************************************************/

io_burst(fd, flag)
int fd, flag;
{
	register int i;
	struct stat fd_stat;
	struct stat tmp_stat;
	int maxfd;


	/* is this a reasonable fd? */
	if (fstat(fd, &fd_stat) == -1) {
		errno = EBADF;
		return -1;
	}

	open_fds[fd] = &dil_fds[fd];

	/* we only map/unmap hpib dil files */
	if (((fd_stat.st_mode & IFMT) != IFCHR) || /* not a character dev */
	    ((major(fd_stat.st_rdev) != 21) &&     /* not an hpib dev */
	    (major(fd_stat.st_rdev) != 22))) {      /* not a gpio dev */
		errno = ENOTTY;
		open_fds[fd]->d_sc = NULL;
		return -1;
	}
	open_fds[fd]->d_sc = &isc_info[m_selcode(fd_stat.st_rdev)];

	/* if flag is non zero then map the card in, else unmap it */
	if (flag) {

		/* if this file is already mapped then return */
		if (dil_fdp[fd]) {
			return(0);
		}

		if (map_on) {
			/* 
			** this process has at least one mapped file.
			** if the file we are trying to map is on
			** the same select code as a mapped one
			** then mark it as mapped and return.
			*/
			maxfd = getnumfds();
			for (i = 0; i <= maxfd; i++) {
				/* don't check fd against nofiles or itself */
				if ((open_fds[i] == 0) || (fd == i))
					continue;

				/* if its not mapped the forget it */
				if (!dil_fdp[i])
					continue;

				/* is this an ok open file? */
				if (fstat(i, &tmp_stat) == -1)
					continue;

				/* same select code? */
				if (m_selcode(tmp_stat.st_rdev) ==
				     m_selcode(fd_stat.st_rdev)) {
					/* already mapped so mark it */
					dil_fds[fd] = dil_fds[i];
					dil_fdp[fd] = &dil_fds[fd];
					dil_fds[fd].ba = m_busaddr(fd_stat.st_rdev);
					/* get state, pattern, ... */
					ioctl(fd, IO_REMAP, &dil_fds[fd]);
					/* we are mapped in */
					return 0;
				}
			}
		}

		/* try to lock the select code */
		if (io_lock(fd) == -1) {
			return -1;	/* can't lock the bus */
		}

		/* now try to map the card in to our space */
		if (ioctl(fd, IO_MAP, &dil_fds[fd]) == -1) {
			io_unlock(fd);	/* give the bus back */
			return -1;	/* can't map in the card */
		}
		dil_fdp[fd] = &dil_fds[fd];	/* mark this one */
		dil_fdp[fd]->d_sc = &isc_info[m_selcode(fd_stat.st_rdev)];
		dil_fdp[fd]->d_sc->mapped = 1;	/* mark this one */
		dil_fdp[fd]->d_sc->card_type = dil_fdp[fd]->card_type;

		if (dil_fdp[fd]->temp & TI9914_HOLDOFF)
			dil_fdp[fd]->d_sc->state |= TI9914_HOLDOFF;
		else
			dil_fdp[fd]->d_sc->state &= ~TI9914_HOLDOFF;
		if (dil_fdp[fd]->temp & PARITY_CTL)
			dil_fdp[fd]->state |= PARITY_CTL;
		else
			dil_fdp[fd]->state &= ~PARITY_CTL;

		if (dil_fdp[fd]->card_type == INTERNAL_HPIB)
			dil_fds[fd].cp += 0x8011;
		if (dil_fds[fd].card_type == HP98624)
			dil_fds[fd].cp += 0x11;

		/* if we got here, then the card has been mapped in */
		map_on = 1;	/* now let the world know */
	} else {	/* unmap the card */

		/* if it wasn't mapped in then return */
		if (!dil_fdp[fd]) {
			return(0);
		}

		if (unmap_check(fd)) {
			return 0;
		}

		/* now try to unmap the card */
		if (ioctl(fd, IO_UNMAP, &dil_fds[fd]) == -1) {
			return -1;	/* can't unmap in the card */
		}

		/* clear these flags before calling io_unlock; see DSDe400123 */
		dil_fdp[fd]->d_sc->mapped = 0;
		dil_fdp[fd] = 0;

		if (io_unlock(fd) == -1) {
			return -1;	/* can't unlock the bus */
		}

		map_on = 0;	/* turn off unless someone is still there */
		maxfd = getnumfds();
		for (i = 0; i <= maxfd; i++)
			if (dil_fdp[i] != (struct fd_info *) 0)
				map_on = 1;	/* There still are some */
	}
	return 0;
}

unmap_check(fd)
{
	register int i;
	int maxfd;

	maxfd = getnumfds();
	for (i = 0; i <= maxfd; i++)
		/* see if there are any other fd's still mapped */
		if ((fd != i) && (dil_fdp[i] != (struct fd_info *) 0))
			if (dil_fds[fd].cp == dil_fds[i].cp) {
				dil_fdp[fd] = 0;
				/* let the kernel know the file is unmapped */
				ioctl(fd, IO_UNMAP_MARK, &dil_fds[fd]);
				return 1;
			}
	return 0;
}

/* this table is for quick calculation of odd parity */
/*     used by drivers (ti9914.c)	*/

char odd_partab[] = {
	0200,0001,0002,0203,0004,0205,0206,0007,
	0010,0211,0212,0013,0214,0015,0016,0217,
	0020,0221,0222,0023,0224,0025,0026,0227,
	0230,0031,0032,0233,0034,0235,0236,0037,
	0040,0241,0242,0043,0244,0045,0046,0247,
	0250,0051,0052,0253,0054,0255,0256,0057,
	0260,0061,0062,0263,0064,0265,0266,0067,
	0070,0271,0272,0073,0274,0075,0076,0277,

	0100,0301,0302,0103,0304,0105,0106,0307,
	0310,0111,0112,0313,0114,0315,0316,0117,
	0320,0121,0122,0323,0124,0325,0326,0127,
	0130,0331,0332,0133,0334,0135,0136,0337,
	0340,0141,0142,0343,0144,0345,0346,0147,
	0150,0351,0352,0153,0354,0155,0156,0357,
	0160,0361,0362,0163,0364,0165,0166,0367,
	0370,0171,0172,0373,0174,0375,0376,0177
};


int (*(dil_handlers[NUMFILE]))();
static void catch_dil_sig();

io_interrupt_ctl(fd, flag) /* enable/disable user interrupts */
int fd;
int flag;
{
	struct ioctl_type arg;

	arg.sc_state = 0;
	arg.type = IO_INTERRUPT;
	arg.data[0] = (flag ? SET : CLEAR);
	return(ioctl(fd, IO_CONTROL, &arg));
}

int (*dil_signal(action, eid, signal))()
register int (*action)();
register int eid;
register int signal;
{
	struct sigvec vec, ovec;
	int old_mask;
	int (*old_action)();

	vec.sv_mask = 0;
	vec.sv_onstack = 0;
	old_mask = sigblock(-1L);
	vec.sv_handler = catch_dil_sig;
	if (sigvector(signal, &vec, &ovec) == 0) {
		old_action = dil_handlers[eid];
		dil_handlers[eid] = action;
		sigsetmask(old_mask);
		return(old_action);
	} else {
		sigsetmask(old_mask);
		return((int (*) ())-1);
	}
}


int (*io_on_interrupt(fd, causevec, handler))() /* set up user interrupt */
int fd;
struct interrupt_struct *causevec;
int (*handler)();
{
	struct ioctl_intr_type arg;
	struct ioctl_type arg2;
	int (*old_handler)();
	int error = 0;
	int signal;
      
	/* set interrupt signal to new signal */
	error = ioctl(fd, HPIB_SET_DIL_SIG, &arg2); 
	if (error == error_value) 
		return((int (*)())error);
	signal = arg2.data[0];

	arg.eid = fd;
	arg.cause = causevec->cause;
	arg.mask = causevec->mask;
	arg.proc = handler;

	/* validate users request */
	error = ioctl(fd, HPIB_INTERRUPT_SET, &arg);
	if (error) return((int (*)())error);

	/* set up the interrupt handler */
	old_handler = dil_signal(handler, fd, signal);

	/* enable users request */
	error = ioctl(fd, HPIB_INTERRUPT_ENABLE, &arg);
	return((error == error_value) ? (int (*)())error_value : old_handler);
}

struct	interrupt_struct intr_causevector;
struct	interrupt_struct *intr_causevec = &intr_causevector;

static void catch_dil_sig(signo, code, scp)
int signo, code;
struct sigcontext *scp;
{
	int intr_eid;

	intr_eid = scp->sc_pad1;
	intr_causevec->cause = code >> 8;
	intr_causevec->mask = scp->sc_pad2;
	(*(dil_handlers[intr_eid]))(intr_eid, intr_causevec);
}


get_open_fd_info(fd)
{
	struct stat fd_stat;

	if (fstat(fd, &fd_stat) == -1) {
		errno = EBADF;
		return -1;
	}
	open_fds[fd] = &dil_fds[fd];
	if (((fd_stat.st_mode & IFMT) != IFCHR) || /* not a character dev */
	    ((major(fd_stat.st_rdev) != 21) &&     /* not an hpib dev */
	    (major(fd_stat.st_rdev) != 22))) {      /* not a gpio dev */
		open_fds[fd]->d_sc = NULL;
		return;
	}
	open_fds[fd]->d_sc = &isc_info[m_selcode(fd_stat.st_rdev)];
}

io_check(fd)
register int fd;
{
	register struct fd_info *fdp = open_fds[fd];

	/* if this is not a dil device file return 0 */
	if ((fdp == NULL) || (fdp->d_sc == NULL))
		return 0;

	/* is this file mapped? */
	if (dil_fdp[fd])
		return 1;

	/* file isn't mapped - is select code? */
	if (fdp->d_sc->mapped)
		if (fdp->d_sc->card_type == INTERNAL_HPIB ||
		    fdp->d_sc->card_type == HP98624)
			return -1;

	/* not mapped */
	return 0;
}

dil_ioctl_check(fd, cmd, arg)
register int fd;
register int cmd;
register struct ioctl_type *arg;
{
	register struct fd_info *fdp = open_fds[fd];

	/* if this is not a dil TI9914 device file return 0 */
	if ((fdp == NULL) || (fdp->d_sc == NULL))
		return 0;

	if (arg == NULL)
		return 0;

	arg->sc_state = 0;

	/* is select code mapped? */
	if (fdp->d_sc->mapped) {
		if (fdp->d_sc->card_type == INTERNAL_HPIB ||
		    fdp->d_sc->card_type == HP98624) {
			switch(cmd) {
				case IO_CONTROL:
				case IO_STATUS:
				case HPIB_CONTROL:
				case HPIB_STATUS:
				case HPIB_SEND_CMD:
				case HPIB_INTERRUPT_ENABLE:
					arg->sc_state = fdp->d_sc->state | PASS_IN;
					return 1;
			}
		}
	}

	/* not mapped */
	return 0;
}

dil_ioctl_set(fd, arg)
register int fd;
register struct ioctl_type *arg;
{
	arg->sc_state &= ~PASS_IN;
	if (arg->sc_state & PASS_OUT) {
		arg->sc_state &= ~PASS_OUT;
		open_fds[fd]->d_sc->state = arg->sc_state;
	}
}

dil_set_isc_state(fd)
register int fd;
{
	if (open_fds[fd]->d_sc->state)
		ioctl(fd, HPIB_SET_STATE, 0);
	else
		ioctl(fd, HPIB_CLEAR_STATE, 0);
}

dil_get_isc_state(fd)
int fd;
{
	ioctl(fd, HPIB_GET_STATE, 0);
	if (errno == 2)
		open_fds[fd]->d_sc->state = 1;
	else {
		if (errno == 1)
			open_fds[fd]->d_sc->state = 0;
		else
			printf("Drew screwup!\n");
	}
}

close(fd)
int fd;
{
        /* check for a valid file descriptor */
        if (fd < 0 || fd > NUMFILE)
            return(dilbadfd(fd));

	/* unmap if necessary */
	unmap_check(fd);

	/* null out open fd ptr field and fd_info struct */
	open_fds[fd] = NULL;
	memset(&dil_fds[fd], '\0', sizeof(struct fd_info));

	/* call the kernel's close (via __close in dillibs.s) */
	return(_close(fd));
}

