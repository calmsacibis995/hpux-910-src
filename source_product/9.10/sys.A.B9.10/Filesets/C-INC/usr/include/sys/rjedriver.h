/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/h/RCS/rjedriver.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:01:40 $
 */
/* @(#) $Revision: 1.3.84.3 $ */       
#ifndef _SYS_RJEDRIVER_INCLUDED /* allows multiple inclusion */
#define _SYS_RJEDRIVER_INCLUDED
#ifdef __hp9000s300

#define MAX_RDWR_SIZE	768	/* Max amount of read/write data allowable =  */
				/* 2 of PDI buffers (3*256=768 bytes each)    */
#define MAX_AEV_SIZE	3	/* Max # of bytes in any AEV block for RJE;   */
				/* this may change for other protocols someday*/

struct RQB_type  {		/*Request block written to PDI card	*/
	unsigned short	 transaction_id;
	unsigned char	 request_code;
	unsigned char	 func_code;
	unsigned char	 port_id;
	unsigned short	 data_length;
	unsigned char   *other_params;
};


struct RSB_type  {		/*Result Block of the completed transaction-  */
				/* read from PDI.	*/
	unsigned char	status;
	unsigned short	transfer_length;
	unsigned char	other_data[4];
};

struct AEV_type  {		/*When async. event interrupt occurs, this is */
				/* read from PDI.	*/
	unsigned char	event_code;
	unsigned char	port_id;
	unsigned char	other_info[MAX_AEV_SIZE - 2];
};

struct RJ_data_type	{
	unsigned char	 *iocard;		/* Address of card	      */
	short		 FATAL;			/*0, normal or 1, card report-*/
						/*ed fatal error, all tranx to*/
						/*card are blocked until next */
						/*open.			      */
	char		 port_id;		/* 0(Normal) or 1(Trace) port */
	char		 driverstate;		/* States:		      */
						/*0:/dev file "closed",	      */
					   	/*1:"read" awaiting interrupt,*/
					   	/*2:"write" awaiting interrupt*/
					   	/*3:"ioctl" awaiting interrupt*/
					   	/*4:"open" awaiting RESET to  */
						/*   complete,		      */
					   	/*8:Nobody waiting on anything*/
					   	/*9:"open" pending	      */
	char		 errornum;		/* States:		      */
	unsigned short	 read_type_state;	/* These 2 'states' are set by*/
	unsigned short	 read_subf_state;	/*  ioctl(fd,50,buf) to equal */
						/*  buf[0], buf[1]. Subsequent*/
						/*  read's pass these to card */
						/*  as xaction, subfunc. #'s  */
	unsigned short	 write_type_state;	/* These 2 'states' are set by*/
	unsigned short	 write_subf_state;	/*  ioctl(fd,51,buf) to equal */
						/*  buf[0], buf[1]. Subsequent*/
						/*  write's pass these to card*/
						/*  as xaction, subfunc. #'s  */
	struct RQB_type  xaction;		/* Transaction to be written  */
						/*  to card.		      */
	short		 semafore;		/* Semaphore for read/write/  */
						/*  ioctl to sleep() on while */
						/*  awaiting xactions done.   */
   						/*  rjeintr will wakeup() them*/
						/*  when xactions are done.   */

	struct RSB_type  last_RSB[10];		/* The last result status blk */
						/*  recvd for each of the 9   */
						/*  transaction types	      */
	struct AEV_type  last_AEV;		/* Last Asynchronous EVent    */
	unsigned char	*usr_databuf;		/* Adr of read/write buf in   */
						/*  user space		      */
	unsigned short	 data_count;		/* 'Return value' passed back */
						/*   to user		      */
	unsigned char	 self_test;		/* open() reads this from card*/
	unsigned char	 test_connector;	/* open() reads this from card*/
	unsigned char	 internal_error[5][11];	/* More detailed error info.  */
	unsigned char	 rqb_misc_bits;		/* See rjcopy_RQB_to_card()   */
	unsigned char	 driv_databuf[MAX_RDWR_SIZE];	/* Driver's read/write*/
							/*  buffer	      */
 };



struct RJ_stat_type {			/* Each PDI/RJE card has 2 ports which*/
					/* can be simultaneously by the driver*/
	struct RJ_data_type	*port0;	/* Example: /dev/rje20 has Major,Minor*/
	struct RJ_data_type	*port1;	/* numbers of 15,20 and maps to port 0*/
					/* while /dev/rjetrace20 has Maj,Min  */
					/* nos. of 15,52 (=20+32) and maps to */
					/* port 1.  Both ports are on select  */
					/* code 20.			      */
	unsigned int		nusers; /* Number of users of the card= 0,1,2 */
};


struct STB_type  {		/*Intermediate status of ongoing xaction read */
				/* from PDI card	*/
	unsigned char		transaction_status;
	unsigned short		transaction_id;
};
#endif /* __hp9000s300 */
#endif /* _SYS_RJEDRIVER_INCLUDED */
