/*
 * @(#)<filename>: $Revision: 1.1.83.3 $ $Date: 93/09/17 16:26:26 $
 * $Locker:  $
 */

/* RTIprefix.h
Include file for rti0.c	HP-UX host  driver,  describing	 the  defined  prefixes
(which	consist	 of  PREFIXSIZE	 8-bit bytes) which are transferred between the
host system and the RTI system. This is also the include file for  bpio.c,  the
RTI backplane driver.

RCS information:
@(#) $Revision: 1.1.83.3 $
     $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/io/RCS/an_rti.h,v $
     $Date: 93/09/17 16:26:26 $
*/
/************************* Miscellaneous Notes *********************

The RTIpids used in backplane communication are 16 bit values.	pSOS  pids  are
actually defined to be 32 bit values, but we can reduce the pids to 16 bits, to
compact data flow. If this ever becomes a problem, we can create a table  which
converts  16  bit  indexes  to	32  bit	 pids. For this reason, the RTIpids are
defined as unsigned short (16 bits), to facilitate  easy  host	conversions  to
ints. 

Beware of the byte switching "features" when converting from host to  RTI.  The
host  defines  a  long	as  msb,  ...  lsb,  while RTI defines this as lower16,
upper16, where each 16 bits is in the form (lsb,msb).	This  will  place  some
extra  overhead	 on the host, since it will need to do the byte translation. At
this time it is not defined how the two compilers allocate bit fields. For this
reason, all prefixes will be padded to PREFIXSIZE bytes.

/************************* Description of Protocol *********************
The protocol between RTI and the host driver consists of 4 parts:

1) host driver sending a prefix to the RTI backplane driver
2) RTI backplane driver sending entire prefix queue to the host driver
3) host driver forcing the RTI system into the bootstrap loader
4) DMA transfer between the host and RTI

Each of these four modes are explained below.

PREFIX HOST DRIVER -> RTI BP DRIVER:
	The  backplane	driver	is  normally  configured  for  a  DMA  transfer
	host->RTI of PREFIXSIZE bytes. When this transfer is satisfied, the RTI
	backplane driver interprets the DMA data as a host prefix, and performs
	the desired function.

PREFIX QUEUE RTI BP DRIVER -> HOST DRIVER:
	Whenever the RTI backplane driver wants to send one  or	 more  prefixes
	(RTI  prefixes),  it  must first assert VBI_INT, interrupting the host.
	When the host receives this interrupt, it will send a "ISHOSTSENDQUEUE"
	HOSTprefix  to	RTI,  indicating  that the RTI backplane driver may now
	transfer prefixes to the host.  The  RTI will then send its entire 
	prefix queue, then configure itself back for a DMA read for one 
	HOSTprefix. Then the host driver  will  act on  the prefixes, 
	performing the requested function.

HOST DRIVER ASSERTING SOFT_RESET LINE:
	This is the method the host driver uses to regain  control  of	a  run-
	away  RTI  processor,  or to perform initial software load when the RTI
	backplane driver is not yet loaded. When this line is asserted, the RTI
	subsystem executes the NMI interrupt service routine. This  will  cause
	the  bootstrap loader to become invoked, and the RTI system will assert
	VBI_INT,  and  transfer	 an  "ISRTIENTERBOOT"  RTI  prefix,  indicating
	response  to  SOFT_RESET.  The	RTI subsystem will then be ready to DMA
	input PREFIXSIZE bytes of data. It loops  at  the  address  defined  in
	rtiio.h	 (RTIHALT_ADDR).   The value returned back from a soft reset is
	determined by the state of the jumper on the card. If  so configured,  
	soft reset will actually cause a reset signal, so the RTI processor will
	be reset. In this case the bootstrap loader will return an 
	"ISRTIENTERBOOT" prefix, indicating power on.

DMA TRANSFER:
	With some prefixes, data is  to	 be  transferred.  This	 will  be  done
	immediately  after  the	 prefix	 is  transferred.  When the transfer is
	complete, the RTI subsystem will return to it's idle state, waiting for
	a DMA input transfer of PREFIXSIZE bytes.


/************************* PREFIXES RTI -> HOST *************************/
/*		 (commonly referred to as RTI prefixes)			*/
#ifndef __RTIPREFIX
#define __RTIPREFIX

#define PREFIXSIZE	16	/* size in bytes of each prefix */
#define MAXPREFIXSIZE	64	/* maximum number of prefixes in the bp queue */
#define BSLPREFIXSIZE	16	/* number of prefixes in the bootstrap queue */

typedef struct {
#define RPRFXIDSIZE 0xF	/* size of RTI prefix ID */
	unsigned char	RTIprefixid;	/* indication of what type of prefix */
	/* Valid RTI prefixes defined (RTI -> HOST)			*/
#define ISRTIENDOFQUEUE 0	/* end of RTI prefix queue		*/
#define ISRTIENTERBOOT	1	/* RTI entered bootstrap loader		*/
#define ISRTIXFER	2	/* request by RTI process to xfer data	*/
#define ISRTISIGMESG	3	/* user defined pSOS generated signal	*/
#define ISRTIDELETE	4	/* RTI delete in progress		*/
#define ISRTIERROR	5	/* RTI detected an error		*/
#define ISRTIABORT	6	/* Abort a particular request		*/
#define ISRTIBPQSIZE    7       /* RTI backplane driver prefix queue size */
/*
Priority of RTI process associated with this prefix: 255 is highest priority, 1
is  the	 lowest	 priority,  and 0 is reserved for special priority (request not
associated with a process, such as bootstrap replies, and  requests  by	 pROBE.
The host driver therefore considers 0 to be of higher priority than 255.
*/
	unsigned char	priority;
} RTIheadertype;




/***** Type ISRTIENTERBOOT: RTI system entered the bootstrap loader *****/
typedef struct {
	RTIheadertype	header;		/* standard header for RTI prefixes */
	unsigned char	enterreason;	/* Reason for entering the bootstrap 
					   loader. */
#define POWER_ON	1		/* Entered due to a power on or
					   pulse on the reset line.  "result"
					   is invalid. */
#define DESTDIAG	2		/* Entered bootstrap loader due to a
					   jump to the destructive diagnostics.
	    				   Results are available in "result" */
	unsigned char rtimemsize;       /* size of RTI memory in 4K chunks. */
	unsigned short core_diag;	/* Diagnostic results of RTI core   */
	unsigned short top1_diag;	/* Diagnostic results of SBX card 1 */
#ifdef hp9000s300
	unsigned short top2_diag;	/* Diagnostic results of SBX card 2 */
#endif
	char	pad2[8];
} RTIenterboottype;



/****** Type ISRTIXFER: Request to transfer data to/from host *****************/
typedef struct {
	RTIheadertype	header;		/* standard header for RTI prefixes */
	unsigned char	RTIwrite;	/* if TRUE, transfer is RTI->host */
	unsigned char	blocking;	/* Blocking request if TRUE	*/
/*
Non-blocking  read  requests: (blocking	 ==  FALSE)  Requests	must   complete
immediately.   If no data is available, return 0; if less than requested length
available, return what is there,  if  >=  requested  length  available,	 return
requested  len.	  Blocking read  requests: (blocking == TRUE) Requests complete
only when the requested amount of data is available.  Blocking and non-blocking
write requests: requests complete only when all of the data has been transferred
to the host.
*/
	unsigned short	RTIpid;	/* process ID of RTI process.  If 0, indicates
				   request is from pROBE.  */
	unsigned short	length;	/* length of request in bytes */
	unsigned long	RTIaddr;	/* RTI address of data transfer	*/
	unsigned short	transaction;
/*
If the request is from pROBE, the first transaction after a pROBE break
will have transaction number 0,  all others are non-zero.  If from a process,
this will be the number given by the IO system.
*/
	unsigned char	index;	/* Two parts which make up the key used by the
				   IO system to determine the related
				   outstanding request */

#ifdef hp9000s300
	char    header_byte;	/* Contains the first byte of the transfer and
				   its usage is up to the host driver rti0().
				*/	
#else
	char	pad;
#endif hp9000s300
} RTIxfertype;




/***** Type ISRTISIGMESG: Asynchronous event RTI prefix: an RTI process wants to
   signal the host *****/
typedef struct {
	RTIheadertype	header;	/* standard header for RTI prefixes	*/
	unsigned short	RTIpid;	/* process ID of signaling RTI process	*/
	unsigned char	mesg;	/* signaling message (host queues 3 deep) */
	char	pad[11];
} RTIsigmesgtype;




/***** Type ISRTIDELETE: Request to delete an RTI process pending *****/
/*
The target RTI pid is being deleted by the RTI I/O system.  Host  will	perform
the  delete, then reply with a host prefix of type ISRTIDELETE.
*/
typedef struct {
	RTIheadertype	header;		/* standard header for RTI prefixes */
	unsigned short	deletepid;	/* proc ID of RTI proc to be deleted */
	char	pad[12];
} RTIdeletetype;




/***** Type ISRTIERROR: RTI detected an error *****/
typedef struct {
	RTIheadertype	header;	/* standard header for RTI prefixes	*/
	unsigned char	type;	/* type differentiating following fields */
#define ERRPANIC	0	/* print the (up to RTIERRLEN) characters
				   with the parameter.  If message is
				   less than 12 characters, it must be null
				   terminated, else an implied null at the
				   13th position will be created.  Use the
				   panic structure below.  */
#define ERRINTVECT	1	/* the type of interrupt in the intvect
				   structure below occurred */
#define ERRDMATO	2	/* DMA timeout (rtidaemon log) use pram */
#define ERRINTERNAL	3	/* Internal rti0 error (rtidaemon) use pram
				   and message.  */
#define ERRBADCARD      4	/* The RTI card failed identify.  This means
				   that the card is dead, or there is an 
				   incorrect card in the RTI slot, or there
				   is no card present. */
#define ERRFATAL        5       /* The RTI card has encountered an unrecoverable
				   error.  The location of the panic message
				   is passed to the host driver who is 
				   reponsible for retrieving and reporting the
				   message to the user. */
#define ERR80C187       6	/* An 80C187 numeric coprocessor exception 
				   condition has occurred */
#define RTIERRLEN PREFIXSIZE - sizeof(RTIheadertype) - 2 /* max msg length */
	unsigned char	pram;	/* parameter with message (ERRPANIC), or
				   interrupt type (ERRINTVECT), or driver
				   error pram (ERRINTERNAL), or type of
				   prefix (ERRDMATO) being sent during DMA
				   timeout, or size of fatal error string  */
	union {
		char	message[RTIERRLEN];	/* ERRPANIC, ERRINTERNAL */
		struct {
			/* address of the next instruction to be executed or
			   a pointer to the fatal error string */
			unsigned short	segment;
			unsigned short	offset;	
		} interrupt; 			/* ERRINTVECT or ERRFATAL only*/
		struct { /* 80C187 exception information */ 
			unsigned long ip;	/* 20-bit instruction pointer */
						/* physical addrs of faulty   */
						/* instruction*/
			unsigned long op;	/* 20-bit operand pointer --  */
						/*phys addrs of operand if any*/
			unsigned short opcode;  /* opcode of faulty instructn */
			unsigned short status;  /* status word containing     */
						/* exception flags & reg top  */
		} fp;
	} u;
} RTIerrortype;




/***** Type ISRTIABORT: RTI received a driver_abort entry *****/
/* Host will either ignore this request if there is no such request to complete,
   or it will complete the request prematurely	*/
typedef struct {
	RTIheadertype	header;		/* standard header for RTI prefixes */
	unsigned short	RTIpid; 	/* RTI process ID which to abort */
	unsigned char	abort;		/* if TRUE, abort, not timeout	*/
	unsigned char	aborttype;	/* read or write		*/
#define READ	1
#define WRITE	2
/*
No abort is performed on  IOCTL,  OPEN,	 or  CLOSE.  If	 the  backplane	 driver
receives  a  driver_abort  entry  for  either  of these three requests, it will
ignore the request, since it should soon be completed.
*/
	unsigned short transaction;	/* transaction number of request we
					   want to abort.  If this does not
					   match the transaction number of the
					   current queued request in the rtiio
					   driver, this prefix is ignored.  
					   This prevents us from aborting the
					   wrong request */


	char	pad[8];
} RTIaborttype;



/***** Type ISRTIBPQSIZE: Inform host driver of RTI prefix queue size *****/
/* This prefix will be sent when switching from the hardcoded 16 prefix
queue size in the bootstrap loader to the user-configured prefix queue size
used by the backplane driver. */

typedef struct {
	RTIheadertype	header;		/* standard header for RTI prefixes */
	unsigned char	bpqsize;	/* */
	char  		pad[13];
} RTIbpqsizetype;



/***** union allowing reference to all RTI prefixes *****/
typedef struct {
	union {
		RTIheadertype		RTIheader;
		RTIsigmesgtype		RTIsigmesg;
		RTIenterboottype	RTIenterboot;
		RTIxfertype		RTIxfer;
		RTIerrortype		RTIerror;
		RTIaborttype		RTIabort;
		RTIdeletetype		RTIdelete;
		RTIbpqsizetype 		RTIbpqsize;
		char			buf[PREFIXSIZE];
	} u;
} RTIprefixtype;






/************************* PREFIXES HOST -> RTI *************************/
/*		 (commonly referred to as HOST prefixes)		*/
/* All possible HOST prefixes defined (HOST -> RTI) */
#define ISHOSTGENEVENT	1	/* generate a pSOS event */
#define ISHOSTJUMP	2	/* jump to a specified location.  It is 
				   not required that we are in the BSL
				   to make this request.  */
#define ISHOSTPROBEBREAK 3	/* generate a break to pROBE for attn.	*/
#define ISHOSTSENDQUEUE 4	/* send all (NUMRTIPREFIX) RTI prefixes */
#define ISHOSTXFER	6	/* satisfy a pSOS read/write request, online
				   memory load/dump, bootstrap memory load/
				   dump, or satisfy a pROBE read/write
				   request	*/


/* Type ISHOSTGENEVENT: generate an event to the desired process	*/
typedef struct {
	unsigned char	HOSTprefixid; /* indication of prefix type */
	char	pad1;
	unsigned short	RTIpid; /* RTI process ID which to send event */
	unsigned short	eventbits;	/* bits to set for event to process */
	char	pad2[10];
} HOSTgeneventtype;




/* Type ISHOSTJUMP: perform a jump to a specified location	*/
typedef struct {
	unsigned char	HOSTprefixid; /* indication of prefix type */
	char	pad1[3];
	unsigned long	RTIaddr;	/* address to jump to */
	char	pad2[8];
} HOSTjumptype;




/* Type ISHOSTXFER: transfer data (host->RTI/RTI->host)	*/
typedef struct {
	unsigned char	HOSTprefixid; /* indication of prefix type	*/
	/* The following field is only used for writes (hostwrite = TRUE).
	   In CIO we need three bytes for padding.  However, DIO only requires
	   one byte and we use one of the other two bytes to help with host
	   misalignment problems.
	*/	
#ifdef hp9000s300
	unsigned char   header_byte; /* byte appended if odd start address. */
	unsigned char   trailer_byte; /* byte appended if odd length. */
	unsigned char   pad;	     /* Need to be same length as CIO. */
#else
	unsigned char	trailer_bytes[3]; /* bytes to append (if TRAILERBYTES 
					     field > 0) */
#endif hp9000s300
	unsigned short	transaction; /* unique transaction number	*/
	unsigned char	index;	/* Two parts which make up the key used by
				   the IO system to determine the related
				   outstanding request			*/
	unsigned char	flags;
/* ----------------- Definition of bit fields of flags element ------------- */
/*
If the HOSTWRITE bit of flags field is set, the transfer is from host->RTI.  If
it is not set (hostread), the transfer is from RTI->HOST.
*/
#define HOSTWRITE 0x80			/* if set: xfer host->RTI */
/*
The XFERTYPEMASK field specifies which type of transfer this is. It can	 be  of
four different types: transfer to/from memory, to/from pROBE, or to/from a user
process (last transfer), or to/from a  user  process  (partial	transfer).  The
transaction and index fields, as well as the TIMEDOUT and RABORTED bits are only
valid if the XFERTYPEMASK is PARTIALIO or LASTIO. The TRAILERBYTES bits  are  
always valid
*/
#define XFERTYPEMASK	0x60	/* bit fields for following 4 fields	*/
#define LASTIO		0x60	/* final transfer to RTI process, update the
				   counter by length+TRAILERBYTES using 
				   iodone(key, length+TRAILERBYTES TRUE), if
				   host write, or wordlen/2-numextra if read.
				   The ERRORTYPEMASK field below is valid. */
#define PROBE		0x40	/* transfer is to/from pROBE		*/
#define PARTIALIO	0x20	/* partial transfer to RTI process, update the
				   counter by length+TRAILERBYTES using 
				   iodone(key, length+TRAILERBYTES, FALSE) if a
				   host write, else length-TRAILERBYTES if a 
				   read */
#define MEMORY		0x00	/* transfer is to/from memory, not RTI pid */

/*
The ERRORTYPEMASK field specifies if any error should be returned if the LASTIO
bit  field  in	XFERTYPEMASK  field  was  set.	This  indicates either no error
occurred, the request was aborted due to timeout or  abort,  or	a  non-blocking
read   request	 would	block.	Backplane  driver  should  call	 iodone()  with
appropriate parameter based on this field.
*/
#define ERRORTYPEMASK	0x18	/* bit field for following 4 fields	*/
#define RWOULDBLOCK	0x18	/* read request would block (EWILLBLOCK) */
#define RTIMEDOUT	0x10	/* request timed out (ETMOUT)		*/
#define RABORTED	0x08	/* request was aborted (EABORTED)	*/
#define NOERROR		0x0	/* no error (normal completion)		*/

/*
TRAILERBYTES is the number of extra bytes to place at end (0-3), host writes 
only; or the number of extra bytes which were read to the host to keep transfers
on a 4 byte boundary. The RTI card can determine the transmission log using the
formula xlog = (wordlen/2) + flags&TRAILERBYTES if (flags&HOSTWRITE != 0), or 
xlog = (wordlen/2) - flags&TRAILERBYTES if (flags&HOSTWRITE == 0). To be CIO 
compatible (CIO increases the transfer length from host->RTI by one word, to 
keep the CIO and future versions of the backplane driver identical. This stems 
from the fact that in a host->RTI transfer, RTI must send 1 extra word as per 
the protocol.
*/
#ifdef hp9000s300
#define HEADERBYTE 0x02
#define TRAILERBYTES 0x01
#else 
#define TRAILERBYTES 0x07
#endif hp9000s300

	unsigned short	RTIpid;	/* RTI process ID which to send data.  Valid
				only if XFERTYPE is PARTIALIO or LASTIO	*/
/*
RTI always does DMA in 16 bit words, and to be	CIO compatible, the host always
does DMA  on 32 bit boundaries.  Wordlen is the number  of 16 bit RTI words
(not bytes!) to transfer, not including TRAILERBYTES To transfer 64K, the 
special case of wordlen = 0 (assuming not host write and TRAILERBYTES is 
non-zero, used for host writes of 1-3 bytes), indicates that the transfer is 
actually 64K words.
*/
	unsigned short	wordlen;
	unsigned long	RTIaddr;	/* address to transfer data into */
} HOSTxfertype;




/* Type ISHOSTPROBEBREAK: send a break (gain attention) to pROBE	*/
typedef struct {
	unsigned char	HOSTprefixid; /* indication of prefix type */
	char	pad[15];
} HOSTpROBEbreaktype;






/* Type ISHOSTSENDQUEUE: instruct RTI to send queue of len NUMRTIPREFIX */
/* The rti0.c host driver will set up wordlen to be NUMRTIPREFIX*PREFIXSIZE + 1
   for CIO since CIO requires the DMA request to be one byte more than the 
   actual amount to be transferred, due to hardware limitations.  The adding of
   one for CIO is can be modified if future releases so need it.
*/
typedef struct {
	unsigned char	HOSTprefixid; /* indication of prefix type */
	unsigned char	pad1;
	unsigned short	wordlen;	/* length in words of queue transfer */
	char	pad2[12];
} HOSTsendqueuetype;




/* union allowing reference to all HOST prefixes */
typedef struct {
	union {
#define HPRFXIDSIZE 0xF	/* size of host prefix ID */
		struct {unsigned char HOSTprefixid;} HOSTheader;
		HOSTgeneventtype	HOSTgenevent;
		HOSTjumptype		HOSTjump;
		HOSTxfertype		HOSTxfer;
		HOSTpROBEbreaktype	HOSTpROBEbreak;
		HOSTsendqueuetype	HOSTsendqueue;
		char			buf[PREFIXSIZE];
	} u;
} HOSTprefixtype;




/***** union allowing reference to all prefixes (host and RTI) *****/
typedef struct {
	union {
		RTIheadertype		RTIheader;
		RTIsigmesgtype		RTIsigmesg;
		RTIenterboottype	RTIenterboot;
		RTIxfertype		RTIxfer;
		RTIerrortype		RTIerror;
		RTIaborttype		RTIabort;
		RTIdeletetype		RTIdelete;
		struct {unsigned char HOSTprefixid;} HOSTheader;
		HOSTgeneventtype	HOSTgenevent;
		HOSTjumptype		HOSTjump;
		HOSTxfertype		HOSTxfer;
		HOSTpROBEbreaktype	HOSTpROBEbreak;
		HOSTsendqueuetype	HOSTsendqueue;
		char			buf[PREFIXSIZE];
	} u;
} ANYprefixtype;
#endif	/* not __RTIPREFIX */
/* rtiio.h: include file for RTI (Real-Time Interface) system HP-UX processes */

/**************************************************************************
RCS information:
@(#) $Revision: 1.1.83.3 $
     $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/io/RCS/an_rti.h,v $
     $Date: 93/09/17 16:26:26 $
 * 
**************************************************************************/

/************************************************************************/
/*		    RTI ioctl system call commands			*/
/*									*/
/*  This file should be included by any program that wants to use ioctl */
/*  calls for the RTI (Real Time Interface) card.			*/
/*									*/
/************************************************************************/


/* be sure that rtiprefix.h and ioctl.h have been included */
#ifdef	KERNEL


#else	KERNEL


#endif	KERNEL


#define RTIMAXPROCS	64	/* Maximum # processes on RTI card at once */
#define RTISMESG	0x2	/* RTISIGNAL pram allowing SIGIO on mesg  */
#define RTISDEATH	0x1	/* RTISIGNAL pram allowing SIGIO on death */
#define RTISMASK	(RTISMESG | RTISDEATH)

#ifdef hp9000s300
#define RTIMAXADDR	0xEFFFF		/* Max. RTI address */
#define BOOTCPUMAXFER	(64 * 1024 - 1) /* max CPU byte xfer when in BSL */
#define RTI_MAJOR	31
#else
#define RTIMAXADDR	0x7FFFF		/* Max. RTI address */
#define BOOTCPUMAXFER	(128 * 1024 - 4) /* max CPU byte xfer when in BSL */
#define RTI_MAJOR	42
#endif hp9000s300

#define RTIRAMSIZE	(RTIMAXADDR+1)	/* size of RTI memory in bytes	*/

#define PSOSCPUMAXFER	BOOTCPUMAXFER /* max CPU byte xfer when not in BSL */
/*
Use random numbers for defines so less likely user will pass in a random
address which rtiio may mistake as the intended value. Numbers must fit into a
char.
*/
#define IFLUSH		115	/* flush the input buffer RTIBUF ioctl() */
#define OFLUSH		116	/* flush the output buffer RTIBUF ioctl() */
#define IOFLUSH		117	/* flush input & output buffer RTIBUF ioctl() */
#define BLOCKINGIO	118	/* parameter for RTIIOMODE ioctl() call	*/
#define NONBLOCKINGIO	119	/* parameter for RTIIOMODE ioctl() call	*/
#define SEMIBLOCKINGIO	120	/* parameter for RTIIOMODE ioctl() call	*/
/*
RTIBUFHIGH and HOSTBUFHIGH must be less than BOOTCPUMAXFER. They will determine
the maximum size of the host/rti buffer sizes. The larger the buffer, the
greater the throughput, at the possible expense of real-time.  Real-time is
degraded when doing DMA, since an interrupt from the card will not be decoded
until DMA is finished. The maximum amount of DMA which can be performed in one
transfer is determined by the size of the buffer area. Since DMA runs at about
512 bytes/millisecond (0.5MB/S), a DMA transfer of 64K bytes would take about
128ms. This reduces your RTI real-time response to at best 128ms during such a
transfer. Likewise, if the buffer size is 8K, the real-time response would be
about 16ms.
*/
#define RTIBUFHIGH	(64 * 1024)	/* Max. # of bytes RTI buffering */
#define HOSTBUFHIGH	(64 * 1024)	/* Max. # of bytes HOST buffering */
#define RTIKMEMBYTESMAX	(2 * 1024 * 1024) /* Max. # bytes for all RTI cards */
#define RTIEVENTMASK	0x3FF		/* valid bits which may be set
					   in RTIGENEVENT ioctl call */


/* Ioctl structures used as arguments in RTI ioctl() calls */
/*
RTIdiag structure:
This structure is used to return the diagnostic result obtained from the
bootstrap loader.  If the selftest field is not SELFTESTPASS, the following
errors may be the problem:
	Value: Interpretation
	-----  --------------
	5555h: self-test passed
	0000h: diagnostics never completed
	1Ennh: address bit nn stuck low/shorted
	2Dnnh: address bit nn stuck high/shorted
	4Bnnh: data bit nn stuck low/shorted
	87nnh: data bit nn stuck high/shorted
	69xxh: parity read bad, should have been good
	96xxh: parity read good, should have been bad
	3Cxxh: address bus error or refresh failure
	All others are undefined

The results of the TOP diagnostics are returned in the TOPselftest field.
The values for the TOP diagnostics are only valid if the TOP card is present,
and the proper two jumpers are installed (see TOP hardware manual).  The
information returned by the TOPselftest have the form of the most significant
byte is the TOP read result, and the least significant byte is the TOP write
result. In each 8-bit byte, a bit indicates the success (0) or failure (1) of
the test, with the most significant bit of each byte corresponding to TOP port
7, and the least significant bit of each byte corresponding to TOP port 0.

In DIO RTI there is a possibility of two TOP cards.  Therefore, there are two
TOP card selftest fields for DIO as apposed to CIO where there is only one.
*/
/* possible results from bootstrap loader destructive diagnostics */
#define SELFTESTPASS	0x5555	/* passed selftest */
#define SELFTESTINV	0	/* invalid selftest result */

struct RTIdiag {
/*
#ifdef hp9000s300
	unsigned short  TOP1selftest;
#else
*/
	unsigned short	TOPselftest;	/* results of the optional TOP power on
					selftest.  The TOPselftest field is only
					valid if the top test was executed and
					a TOP card is present.  TOP is the
					hp 8 channel iSBX card which may
					optionally be used with RTI.  */
/*
#endif hp9000s300
#ifdef hp9000s300
	unsigned short  TOP2selftest;
#endif hp9000s300
*/
	unsigned short	selftest;	/* results of the power on selftest */
};


/*
RTIsignal structure: This structure indicates the SIGIO signaling status  of  a
particular  processfds.	  Note	that  multiple signals of the flags will not be
recorded. Therefore it is possible for more than one signal  to	 be  indicated.
Messages are RTI generated signals, and their meaning is RTI process dependent.
Up to 3 messages are queued, and shifted  as  additional  message  signals  are
detected. "mesgoverflow" will be set if 3 additional RTI message signals arrive
after the first SIGIO signal and before a ioctl call is made  to  retrieve  the
signals.
*/
struct RTIsignal {
	unsigned mesgoverflow	: 1;	/* 1: overflowed mesgs, last 3 saved */
	unsigned mesg3		: 8;	/* queued message # 3		*/
	unsigned mesg3valid	: 1;	/* 1: message #3 is valid	*/
	unsigned mesg2		: 8;	/* queued message # 2		*/
	unsigned mesg2valid	: 1;	/* 1: message #2 is valid	*/
	unsigned mesg1		: 8;	/* queued message # 1		*/
	unsigned mesg1valid	: 1;	/* 1: message #1 is valid	*/
	unsigned reserved	: 2;	/* reserved for future use: set to 0 */
	unsigned RTISMESGflag	: 1;	/* field for RTISMESG flag	*/
	unsigned RTISDEATHflag	: 1;	/* field for RTISDEATH flag	*/
};

/*
RTIstatus structure:
This structure contains information about an RTI process file.
*/
struct RTIstatus {
	int	RTIpid;		/* process ID of RTI process	*/
	int	hostdata;	/* amount of host write data	*/
	int	RTIdata;	/* amount of RTI write data	*/
	int	host_bufsize;	/* max host data		*/
	int	RTI_bufsize;	/* max RTI data			*/
	int	qreadlen;	/* size of queued RTI read req., 0 if none */
	int	qwritelen;	/* size of queued RTI write req., 0 if none */
	int	iomode;		/* BLOCKING,NONBLOCKING,SEMIBLOCKING */
	int	sigcond;	/* conditions causing SIGIO	*/
	int	sigproc;	/* process receiving SIGIO (0=none) */
	int	dying;		/* process will die when data read */
	unsigned short	reserved[24];	/* reserved for HP */
};

/*
RTIpidmap structure:
This structure and the RTIdaemon structure are only for use by the RTI daemon
process.  This structure is used to pass the proposed RTI process id (RTIpid)
which was returned upon a pSOS SPAWN_P call, and to receive a major and minor
number which is used by the rtidaemon process, using mknod(2), to create a
character special device file.
*/
struct RTIpidmap {
	unsigned short	RTIpid;	/* the RTI pid which was returned from a
				previous spawn_p(pSOS) call (INPUT) */
	int	RTI_bufsize;	/* Size of buffer for RTI->host transfers */
	int	host_bufsize;	/* Size of buffer for host->RTI transfers */
	short	major_num; /* the major number to use when creating the
				character special file using mknod(1)
				(OUTPUT) */
#ifdef hp9000s300
	int	minor_num; /* the minor number to use when creating the
				character special file using mknod(1)
				(OUTPUT) */
#else
	short	minor_num; /* the minor number to use when creating the
				character special file using mknod(1)
				(OUTPUT) */
#endif hp9000s300
};

/*
RTIdaemon structure:
This structure and the RTIpidmap structure are only for use by the RTI daemon
process.  This is the structure used to return a list of deleted processes,
used only by the RTIdaemon process to delete process special device files which
correspond to RTI processes which have been deleted. Each call returns at most
RTIMAXPROCS deleted processes, indicated by the parameter num_minor, up to the
the maximum number of processes in the list, RTIMAXPROCS. The major number of
the device file is the same as the major number of the control file. The most
significant bits of the minor number will be the minor number of the control
file (bits 0-7 are the LU number, bits 8-15 (lsb) is given below in
minor_lsb[]).  Additionally, the rtipanic() messages from the RTI card are sent
to the daemon via this structure (as well as all other RTIERROR prefixes). The
daemon must do a read for exactly sizeof(struct RTIdaemon) bytes to the control
file, whenever it gets a SIGIO signal.
*/
#define RTIMAXERR	64		/* max# errors in RTIdaemon structure */
#define RTIFATALLEN	255		/* Length of RTI fatal string. */
struct RTIdaemon {
	unsigned char 	num_minor;	/* # of elements valid in minor_num */
	unsigned char	minor_lsb[RTIMAXPROCS]; /* list of deleted processes */
	unsigned short	pid[RTIMAXPROCS]; /* corresponding pid of processes */
	unsigned short	numRTIerror;	/* # of prefixes in rtierror struct */
	RTIerrortype	err[RTIMAXERR];	/* the error prefixes from RTI card */
	int		overflow;	/* number of err msgs ignored due to
					   overflow (not reading soon enough) */
	unsigned char   fatal_string[RTIFATALLEN]; /* String sent by RTI card
						      when fatal error occurs.
						      Note since variable
						      before is an integer this
						      string is "word" aligned.
						      This is important for DIO.
						   */		
	unsigned char	fatal_flag;	/* Set if fatal_string is valid. */
};



/*
IOCTL parameters defined exclusively for use with RTI I/O:
Refer to the rtiio(7T) man page for explanation of the use of the following  RTI
ioctl(2) calls.
*/

/* ioctl(2) calls designated for the cpu file				*/
#define RTITARGET	_IOW('R', 0, int) /* set target addr.		*/
#define RTIBOOTJUMP	_IOW('R', 1, int) /* bootstrap jump to location	*/
#define RTIHALT		_IO('R', 2)	/* halt (idle) the RTI system	*/
#define RTIDESTDIAG	_IOR('R', 3, struct RTIdiag) /* run dest. diag & halt */

/* ioctl(2) calls designated for process files				*/
#define RTIGSIGSTAT	_IOR('R', 4, struct RTIsignal)	/* get signal status */
#define RTIGENEVENT	_IOW('R', 5, int)	/* generate signal bits	*/
#define RTIBUF		_IOW('R', 6, int)	/* flush/drain input/output buffer */
#define RTISTAT		_IOR('R', 7, struct RTIstatus) /* ret. conf. parameters */
#define RTIENABLESIG	_IOW('R', 8, int) /* set up to receive SIGIO */
#define RTIIOMODE	_IOW('R', 9, int) /* BLOCKING NONBLOCKING SEMIBLOCKING */

/* ioctl(2) call designated for pROBE file				*/
#define RTIBPROBE	_IO('R', 10)	/* send a break to pROBE */

/* ioctl(2) calls designated for the control file (controlfds) 		*/
#define RTIADDPID	_IOWR('R', 11, struct RTIpidmap)	/*New pid get maj/min*/

/* ioctl(2) calls designated for all of the above. */
#define RTIMEMSIZE	_IOR('R', 12, int)	/* Size of RTI memory. */
#define RTICPUOWNER	_IOR('R', 13, int) /* return pid of CPU owner. */

/* The following ioctl(2) calls will be allowed for the CPU, and process files.
*/
#define RTIMAXKMEM	_IOW('R', 14, int) /* Size of kernel memory. */
/**************************************************************************
RCS information:
@(#) $Revision: 1.1.83.3 $
     $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/io/RCS/an_rti.h,v $
     $Date: 93/09/17 16:26:26 $
**************************************************************************/

/************************************************************************/
/*		 rti0_spc.h ------ RTI system space file		*/
/*									*/
/*  This file contains the type definitions for the message structure	*/
/*  and the port data area structure.					*/
/************************************************************************/

/* The following defines how many RTI prefix requests can be queued inside
   the pda of rti0().
*/
#define RTIQELEMENTS	RTIMAXPROCS * 3

/*
rti0_spc.h: port data area -- major data structure for rti0(). This is the area
that  is  actually placed via a #include in the space.h file. Therefore, all of
the  storage  defined  here  resides  in  the  KERNELSPACE,  and  is  therefore
CONTIGUOUS_REAL memory.  This entire file defines one structure, the RTIpda.
*/
typedef struct RTIpda
{
	unsigned char	state;		/* state of the manager */
	unsigned char	lu;		/* logical unit number	*/
	int		mgr_index;	/* Manager index for this instance. */
	int 		bound_to_cam;   /* Set to TRUE if we are bound to the 
					   CAM. */
	unsigned char	xtracio;	/* This is set to 1 for CIO.
					   This number is used to increase
					   the request length of a RTI->host
					   DMA request, to compensate for a BIC
					   bug that was found.  This was done in
					   case RTI was placed on any future
					   I/O backplanes that do not require
					   an extra word to be transferred. */
	unsigned char	hw_addr_1;	/* This device's subchannel number
					   (same as CIO slot number).	*/
	short		do_bind_msg_id;	/* message id of do_bind req */
	short		message_id;	/* message id last request */
	int		transaction_num; /* transaction number always used */
	port_num_type	diag_port;	/* diagnostic port.  0 if none	*/
	port_num_type	config_port;	/* configurator port number	*/
	port_num_type	my_port;	/* This manager's port number	*/
	port_num_type	lm_port;	/* Lower manager's port number	*/
	io_subq_type	lm_req_subq;	/* Lower manager's request subq */
	int		timer_id;	/* timer id from io_get_timer	*/
	int             rtimemsize;     /* Holds size of RTI memory. 	*/
	int		Q_size;		/* Size of RTI queue. 		*/
	union {
		int i;
		struct RTIdiag s;
	} diag_result;	/* result of diagnostics from RTI card */
	/* message which maybe reused if in portserver. This is used the 
	   same  as rti0_msg_type */
	anyptr	reusemsg;

	long	dmaflags;	/* holds DMA flags defined below	*/
#define DMAIO_BUSY	0x1	/* A process currently is currently using the
				   DMA structures in the pda. */
#define DMAIO_WANTED	0x2	/* A process wants the DMA structures, but
				   was unable to obtain to abtain them. */
/* Various flags which indicate the state of the pda.  These are not combined
   into one variable, because of a critical section problem of or'ing bits.  
   This method, the atomic write of a TRUE or FALSE will not need to be 
   protected.
*/
	char pf_inprogress;	/* a pwr failed has been detected and is 
				   progress. */
	char rti_dead;		/* hdwr was found to be dead.
This can only be remedied by an RTIHALT which will try to reset the RTI card.
If the RTIHALT is successful then the card is ready to be booted, otherwise
the card probably has a hardware problem. */

	char controlopen;	/* control file opened		*/
	char cpuopen;		/* cpu file opened		*/
	char probeopen;		/* probe file is open		*/
	char inbootstrap;	/* RTI is in the bootstrap loader. */
	char initialized;	/* port is initialized & bound */
	char dorequestqueue;	/* This flag will be set if RTI wants to send 
				   the prefix queue, but the DMA structures are
				   currently being used. */
	char reuseframe;	/* reuse the frame in portserver */
	char syncwithprobe;	/* ignore probe req's until sync */
	char signaledaemon;	/* signaled daemon that there are requests to
				   process.  This flag will be cleared when
				   the daemon reads the requests. */
	char foundcard;		/* card passed identification.  This does not
				   indicate if the card is good, only if it
				   was detected to be present. */
#define RTI_CARD_NORMAL   0x0
#define RTI_CARD_RESET    0x1
#define RTI_QUEUE_WANTED  0x2
	char resetcard;         /* Used to indicate that the card is in the
				   process of being rebooted.  This will be
				   when the DMA token is currenlty in use and
				   a reset to the card is in progress.
				*/

	int cpupid;		/* pid of the process owning cpu file. */

/********************* Start of DMAio data structures ***********************/
/*
The following data structures (called DMAio  data  structures)	are  semaphored
using  the  macros  DMAIOBUSY() and DMAIOFREE(). Therefore, no access should be
made to any of these  routines	without	 first	gaining	 control  of  them  via
DMAIOBUSY().  Avoid  holding  the token too long, since there is only one token
per pda. Note there are some points in the driver  that	 will  grab  the tokens
without	 calling  the  above macros. The reason is that if the driver is on the
ICS it cannot sleep and since the DMAIOBUSY macro calls sleep if the  token  is
busy,  this  will  panic  on  the ICS. Therefore there are places in the driver
where the flags are modified without the use of the macros.
*/

/*
Vquads for use in sending and receiving prefixes to/from the CAM, and  for  use
to  get	 the  card  ID	number.	 vquad1	 always contains information for a host
prefix, and points to pda->HOSTprefix. vquad2 always contains  information  for
the  RTI  prefix queue, and points to pda->RTIprefix. vquad_data is dynamically
changed to transfer data, vquad1 and vquad2 are set up at  initialization   and
do not change.  Note vquad1 is used for indentification of RTI.  After the
identification is complete vquad1 is defaulted to the HOSTprefix values.
*/
	cio_vquad_type	vquad1, vquad2, vquad_data, vquad_data2;

/*
Make an area in the pda that is CPU aligned to transfer prefixes (4 32-bit
words used in transfers to/from RTI) so they will not need to be re-aligned by
the lower manager. Note that  this  area  is  defined  in  space.h,  so	 it  is
CONTIGUOUS_REAL	 memory, and therefore will be processed faster by the CAM. The
first prefix is	 for  the  HOSTprefix  (outgoing  prefix),  and	 the  remaining
"MAXRTIPREFIX"es (defined in rtiprefix.h) are for RTIprefixes, (incoming
prefixes).  This start of the aligned data areas are pointed to by *HOSTprefix
and *RTIprefix.  They are defined in space.h as:

char	rtiprefix_data_buf1[NUM_RTI0][PREFIXSIZE + (CPU_IOLINE - 1)];
char	rtiprefix_data_buf2[NUM_RTI0][(MAXRTIPREFIX*PREFIXSIZE)+(CPU_IOLINE-1)];
*/

/* pointer to the cache-aligned prefix.  This is used for all HOSTprefix 
   commands sent to the RTI system.  This points to s single prefix.
*/
	HOSTprefixtype *HOSTprefix;

/* pointer to the first cache-aligned RTI prefixes.  This is used for the
   vquad2 buffer address.  
*/
	RTIprefixtype *RTIprefix;

/* Values with which to perform PPDAFREE on when DMA is done,
   if valid == TRUE.  This structure will only be used if we are transferring
   data to/from a PPDA.
*/
	struct {
		unsigned char pp_index;
		char dmabuflag;
		char valid;
	} DMAfree;
	caddr_t	DMAwakeup;	/* wakeup a PPDA sleeper if non-NULL, and
				   if the contents of the address != 0
				   (indicating someone is sleeping on it) */

/* Buffer on which to perform biodone when DMA is done, if DMAfree is NULL.
   This buf structure is mainly used for the cpu file, and ioctl calls that
   require waiting for completion.  "buf" structures are used by physio().
*/
	struct buf	*DMAbiodone;
	char	dmaioctlsleepflag; /* This flag is used by ioctl()'s to 
				      indicate that there is a process
				      sleeping.  If this flag is set to
				      PROCESS_SLEEPING then there is a 
				      sleeping process awaiting DMA completion.
				    */

/*********************** End of DMAio data structures ***********************/


	unsigned target_addr;	/* address for read/write to cpu file.  This
				   contains the address on the RTI card for
				   the next read/write to the cpu file.
				*/

/*
Unique process ID of rtidaemon process (process which has the control file open,
0 if none) 
*/
	struct proc *u_procp;	

/*
Queue of unprocessed RTI prefixes.  First in list is the prefix currently being
processed. This list must be modified while protected by spl5() and splx(),
unless we are in the port server.  Note that the macros RMPENDINGPREF and
RELEASETOPOOL use these structures.  All elements in the RTIprefixQ array are
linked up to the RTIQ pointer at initialization.  This memory can be examined
at any time to guarantee that all elements are linked to either RTIQ or
RTIQfree.  Note there will be (3 * MAXRTIPROCS) queue elements.  This should
be more than enough since the backplane driver can only store MAXRTIPREFIXES.
*/
	struct	RTIQelement {
		struct RTIQelement	*link;
		RTIprefixtype	prefix;
	} RTIprefixQ[RTIQELEMENTS];

	struct RTIQelement	*RTIQ;           /* Pointer to the first
						    queued element. */
	struct RTIQelement	*RTIQfree;	/* Pointer to all the current
						   free queue elements. */
/*
List of deleted processes and RTI error prefixes. This structure  is  what  the
daemon	reads  when  we	 send  it  a  signal indicating there are entries to be
processed. The daemon will get this structure  by  performing  a  read	on  the
control	 file.	The  driver  only excepts reads on the control file with a size
equal to the structure. When adding or deleting from this  list,  protect  with
spl5()	and  splx()  to prevent overwriting by port server.  
*/
	struct	RTIdaemon daemon;

/* Rti0 internal logging mechanism. */
#define MAXLOG 3000
#define LOG_STRING_LENGTH 50
	/* The following structure is used to internally log information 
	   about the driver.  This log will NOT be part of the end product. 
	   It will be conditionally complied in for debug use.   

	   The log buffer will be circular with the next available entry 
	   pointed to by log_counter.  This internal logging mechanism hinders
	   the performance of rti0() it should under no circumstances ever be
	   shipped with the product.
	*/
	struct driver_logging_struct {
		struct log_elements_struct {
			char log_desc[LOG_STRING_LENGTH];
			unsigned short log_line;  /* Log message. */
			unsigned int log_param;  /* Optional Parameter. */
			unsigned int log_misc1;  /* Reserved for future use. */
			unsigned long usec;	/* from ms_getusec */
		} log_info[MAXLOG];
		/* Counter for circular log buffer. The counter will be pointing
		   to the next available entry to log.  Since interrupts will
		   not be off, the preuiomove_log_counter will be set to the
		   value which log_counter was immediately before the uiomove.
		*/
		unsigned preuiomove_log_counter;
		unsigned events;	/* num events since last read */
		unsigned log_counter;   /* Next available entry. */
	} driver_log;

/* buf structure used for transfers to/from the cpu file (directly transferred
   into user space via DMA.
*/
	struct buf	cpubuf;

/*
Map the pSOS process id into the correct  rtiproc  structure.  Free  RTIprocess
structures  are	 indicated  by psos pid's of 0. Included in this structure is a
pointer to some aligned physical memory in which to store the per process  data
(PPDA).	 The  buffer  space used for the per-process data area is allocated via
the use of kmem_alloc(). There is a limit on the size of what can be allocated.
The  user  will	 be allowed to take up to rti0maxkmembytes(defined in rtispc.h)
with the PPDAs. Any request beyond that amount will result in an  ENOMEM  error
from  the driver. The maximum size that a PPDA can be created is limited by the
values RTIBUFHIGH and HOSTBUFHIGH. These values are defined in rtispc.h so that
the user could if he wanted to poke memory and change the values. The number of
RTI processes is limited to RTIMAXPROCS which is currently 64.

This structure is also used for pROBE. The LAST	 process,  rtiproc[PROBEINDEX],
will actually be used for pROBE. It's data structures will be set up upon first
open to pROBE. No card transfers to pROBE can be performed until this open.
*/
	struct RTIprocess {
		unsigned short	psospid;
/*
Pointers tailptr and headptr:
There are two buffers per process, a host write buffer (RTI read buffer), and
an RTI write buffer (host read buffer).	 tailptr points to place to add the
next byte of data into the buffer. headptr points to the place to retreive the
next byte of data from the buffer. If tailptr == headptr, buffer is empty. if
tailptr = endaddr + 1 (one pass the end of the buffer), buffer is full. These
are not circular pointers! To avoid segmented data transfers, if data is to be
transferred into the buffer which would cause the buffer to need to wrap
around, the buffer must first be re- aligned to the head. No need to copy the
data otherwise, since the optimized CAM code will be copying data to align it
anyway. The re-alignment is always done by the code which places data into the
buffer. The RTI buffers look like (HOST buffers are similar):

	      writeaddr[RTI] ->+---------------------+
			       |		     |
			       |		     |
			       |		     |
		headptr[RTI] ->|   RTI write buf     |\
			       |(for RTI->host xfers)| \
			       |		     |  \
			       | (often called PPDA) |   >BUFAVAILABLE()
			       |		     |  /
			       |		     | /
		tailptr[RTI] ->|		     |/
			       |		     |\
			       |		     | \
			       |		     |  > BUFROOM()
			       |		     | /
			       |		     |/
		endaddr[RTI] ->+---------------------+


The BUFAVILABLE() macro tells how much valid data is in the buffer. The
BUFROOM() macro tells how much data may be transfered in. When code wants to
write into the buffer, if the BUFROOM is not big enough, and headptr does not
equal writeaddr (not aligned at top of buffer), then data is moved and re-
aligned to the front of the buffer (ppda). headptr is incremented by reads out
of the buffer, and tailptr is incremented by writes into the buffer.

RTI writes fill up the RTI write buf, incrementing tailptr[RTI]. RTI reads
empty the host write buf, incrementing headptr[HOST]. Similarly, host writes
fill up the host write buf, incrementing tailptr[HOST]. Host reads empty the
RTI write buf, incrementing headptr[RTI]. As can be seen, eventually the two
pointers will drift to the end of the buffer. This does not become a problem
until the "writer" does not have enough room between the "writer" tailptr and
the end of the buffer. If this is the case, the valid data in the buffer is
shifted back to the head of the buffer, adjusting pointers, allowing more room.
If still not enough room (this is due to the buffer being created too small),
as much data as possible is moved. Note that when the data is shifted, the
buffer semaphore flag must be gotten, via PPDABUSY(). This semaphore token must
be obtained before changing any data or the data pointers.  Note that the
semaphore token should be obtained before looking at the size of the pointers,
to prevent the pointers from changing while we are looking at them (possibly
causing size calculations to be wrong).
*/
#define HOST	0	/* the described structure is for host writes */
#define RTI	1	/* the described structure is for RTI writes */
		caddr_t	headptr[2];	/* address to read from */
		caddr_t	tailptr[2];	/* address to write to */
		caddr_t	writeaddr[2];	/* head of the ppda buffer */
		caddr_t	endaddr[2];	/* tail of the ppda buffer */
		int	ppdasize[2];	/* size of the write buffer */
		char	sleepflag[2];	/* for sleeping in procstrat() */
		long	accessflag[2];	/* exclusive access to PPDA used in
					   PPDABUSY() and PPDAFREE()  */
		unsigned char doCheckDMA[2]; /* portserver needs a call to
						checkdma() done */
		unsigned char inCheckDMA[2]; /* Flag to indicate if "user"
						process is in check_dma() code.
					     */		
/*
Since the RTI IO system only allows one read and/or one write at a time, and
the number of RTIQelements in the system is 3*RTIMAXPROCS, we can save the
pending request with a pointer. If no requests are available, the pointers are
NULL.
*/
		/* unprocessed RTI prefix for write to HOST or RTI */
		struct RTIQelement *RTIprefixw2[2];
/*
Unique process ID of process which to signal for this process when a signal is
generated by an RTI process executing the SIGHOST rtiioctl() call.  Setup by
RTIENABLESIG ioctl().
*/
		struct proc	*u_procp;
		struct {
			union {
				struct RTIsignal signal; /* signal info */
				unsigned bitfield;	/* signal info bits */
			}u;
		} signalinfo;
		char	iomode;		/* BLOCKING,NONBLOCKING,SEMIBLOCKING */
/*
flags of events on which to signal u_procp when the appropriate bit is set in
"signal". These flags consist of RTISDEATH and RTISMESG as defined by the
RTIENABLESIG ioctl argument
*/
		short	signalflags;
		long	writev_flags;	/* separate addr from readv for sleep */
#define WRITEV_WANTED	1		/* There is a WRITE request pending */
#define WRITEV_BUSY	2		/* WRITE process file is in use */
		long	readv_flags;	/* readv addr for sleep */
#define READV_WANTED	1		/* There is a READ request pending */
#define READV_BUSY	2		/* READ process file is in use */
		char pdying; 		/* The RTI process is being deleted */
		char aborting;	/* ISRTIABORT prefix received,
					   indicating that the rtiread() or 
					   rtiwrite() call has timed out */
		unsigned char abort_flags; /* Abort flags to indicate type
					      of abort to RTI. */
		char ptoldaemon;	/* signaled daemon this proc died */
		char ptelldaemon;	/* need response from daemon	*/
		char phaltedcard;	/* no more pSOS to talk to	*/
		char procopen;		/* process is open, can't delete */

#define PROBEINDEX RTIMAXPROCS		/* last one is for PROBEINDEX	*/
#define ROOTINDEX 0			/* first one is for ROOTINDEX	*/

		struct proc *r_proc, *w_proc, *e_proc;
		char	s_state;
#define RDCOLL	0x1
#define WTCOLL	0x2
#define EXCOLL	0x4
	} rtiproc[RTIMAXPROCS+1];	/* one extra for pROBE		*/
} rti0_pda_type;
/* ANALYZE START. This is here in order to severe the file for analyze */
/* display latest revision.  Note the module will be rti0header, not rti0 */
#ifndef PRODREV
static char rti0_rev[]="@(#) $Header: an_rti.h,v 1.1.83.3 93/09/17 16:26:26 root Exp $";
#endif PRODREV

/************************************************************************
 *		   REAL TIME INTERFACE (RTI) MANAGER			*
 *			   Monolithic Manager				*
 *			for the HPPA I/O system				*
 *			  running under HP-UX				*
 ************************************************************************

 This manager accepts the following calls from the high-level I/O
 system:
	open
	close
	read
	write
	ioctl
	select

 Messages are the means of communication between this manager and all
 other managers in the low-level I/O system.  This manager supports the
 following messages:
				       lower
	configurator	 message      manager
	~~~~~~~~~~~~  ~~~~~~~~~~~~~   ~~~~~~~
	   ---->      CREATION
	   ---->      DO_BIND_REQ
		      BIND_REQ	       ---->
		      BIND_REPLY       <----
	   <----      DO_BIND_REPLY
		      CTRL_REQ         ---->
		      CTRL_REPLY       <----
		      IO_REQUEST       ---->
		      IO_REPLY	       <----
		      IO_EVENT	       <----
		      ABORT_EVENT      ---->
		      POWER_ON_REQ     <----
		      POWER_ON_REPLY   ---->


 ************************************************************************/

/*------------------------- include files ------------------------------*/
#ifdef RTI0LOG

#endif RTI0LOG










#undef	u			/*     a nuisance from user.h.	We will
					use udot, which is same as u	*/









#ifdef lint
/* Things which lint wants defined, but we do not need otherwise */
struct vnode {int i};
struct pte {int i};
struct rfa_procmem {int i};
struct rfa_inode {int i};
struct netunam {int i};
struct fs {int i};
struct csum {int i};
struct cg {int i};
struct dinode {int i};
struct text {int i};
struct inode {int i};
struct tty {int i};
struct quota {int i};
#endif lint


/*
A message frame consists of a standard message header and a data part. The data
part  is  defined  as a union of structure types, allowing it to be viewed in a
variety of ways according to the value of the msg_descriptor  in  the  standard
message	 header. It is important that all messages be included in the following
structure, since it's sizeof() is used	to  tell  configurator	what  our  port
server's maximum message length will be.
*/

typedef struct {
	llio_std_header_type	msg_header;
	union
	   {
		creation_info_type	creation_info;
		do_bind_req_type	do_bind_req;
		do_bind_reply_type	do_bind_reply;
		bind_req_type		bind_req;
		bind_reply_type		bind_reply;
		cio_dma_req_type	cio_dma_req;
		cio_dma_reply_type	cio_dma_reply;
		cio_io_event_type	cio_io_event;
		cio_ctrl_req_type	cio_ctrl_req;
		cio_ctrl_reply_type	cio_ctrl_reply;
		diag_event_type		diag_event;
	} u;
} rti0_msg_type;
/************************************************************************/

/******************************* DEFINES ********************************/
/* given dev, return LU number where LU number is defined in the uxgen file. 
   From the users perspective its an LU, from the drivers perspective it will
   be the mgr_index assigned during binding.
*/
#define LU(dev)		((minor(dev) >> 8) & 0xFF)

/* given dev, return pSOS process ID, for more information see rti0_open() for
   explanation of an RTI device file. 
*/
#define MINORINDEX(dev)	(minor(dev) & 0xFF)
#define CONTROLFD 0xFF	/* Id for the control file		*/
#define CPUFD	0xFE	/* Id for the cpu file			*/

/*
To ease processing and reduce special case code, if probe is in system, it will
be  the last unit in the rtiproc structure, behaving in some ways like a
process file. Therefore if RTIMAXPROCS is 64  (as  in  most  cases),  the  65th
element in the structure (rtiproc[64]) will be for pROBE.
*/
#define PROBEFD	PROBEINDEX /* Id that refers to pROBE			*/

/* The following are flags to cleanuproc to indicate how it wass called.  If
   the CDAEMON flag is passed, then cleanuproc was called by the rtidaemon.  If
   CRESET is passed, then the call was because of a reset from ioctl.  All other
   calls pass in CNORMAL to inidcate no special calling.  All this allows us
   to determin when the daemon should be notifed to delete a process special
   file.
*/
#define CRESET	0x8
#define CDAEMON 0x4
#define CNORMAL 0x2

/* given dev, return pointer to pda. LU() returns the mgr_index, since 
   mgr_index was swapped in before the driver call was made. (See auto-config
   document)
*/
#define PDA_MAP(dev)	(rti0_pda_map[LU(dev)])

/*
check transparent bit for diag mode. Note that there may be some problems with
the way this is implemented. In rti0_open, we always reject calls to this guy.
It may be that sherlock diagnostics have some problems with this, since they
may want to perform a destructive diagnostic, and cannot do this because the
CPU file is open. At the present time we do not fully understand what is to be
done with sherlock, and so we assume we are not violating any specs. If need
be, we can make this work like an open to the cpu file, and give it all the
same privlidges, and on close do not clear the cpu open flag. If we are in the
middle of a needed cpu transfer, all might go crazy, and the cpu file will be
definitely dead, but hey, sherlock was never designed to be nice to those who
are using the card when sherlock is run.
*/
/* Macro to check if dev has the diagnostic bit set. */
#define RTI_DIAG(dev)	(minor(dev) & 0x800000)


/* Manager state definitions: creation MUST be zero due to the initial
   state of the PDA!  For more details on the states see rti0_lm_reply_msg().
*/

/* The following states are only valid during initialization */
#define CREATION	0	/* We have a valid pda.  We are waiting for
				a message to bind to lower manager	*/
#define BIND_LOWER	1	/* We have received a request to bind to
				lower manager.  We are waiting for the result
				of the lower manager binding		*/
#define UNBIND_LOWER	11

#define IDENT_WAIT	2	/* We are waiting for the identify request
				to complete.				*/
#define INIT_WAIT4RTIP	3	/* We have asked for the prefix queue, and
				are waiting for it to be returned, so we
				can check the selftest status.		*/

/* The following states are only valid after we have been initialized */
#define IO_REQ_READY	4	/* We are idle.  Ready for an IO request */
#define WAIT4DMA	5	/* We have initiated DMA.  We are waiting
				for a reply to indicate if and when our
				request has been completed.  When it is finished
				we can wakeup anyone sleeping on the DMA,
				as well as freeing up the DMAio structures.
				We must initialize pda->DMAbiodone.  */
#define WAIT4PREFIXQ	6	/* We have received an interrupt, and sent
				a prefix to request the entire RTI queue. */
#define SWAIT4ENTERBSL	7	/* We reset the RTI card, and are
				waiting for a prefix indicating that RTI
				entered the bootstrap loader.		*/
#define HWAIT4ENTERBSL	8	/* We have performed a jump to destructive
				diagnostics, and are waiting for a prefix
				indicating that RTI entered the bootstrap
				loader.		*/
#define WAIT4DIAGJUMP	9	/* We just sent a jump to dest diagnostics.
				When it finishes, we will request the prefix
				queue. */
#define WAIT4ERRSTRING	10	/* We are transferring the fata error string
				   from RTI to the host.  After it completes
				   we need to mark the card dead.  */



/*
The rev code will be given to the lower manager with the bind  request	so  the
lower  manager	may want to verify the proper revision. In the same manner, the
subsystem number is passed. Again, this is not used by HP-UX, so we will  leave
it as is.
*/
#define MY_REV_CODE	1111
#define MY_SUBSYS	1400

#define POWER_ON_SUBQ	  0	/* Subq to send power on req's to	*/
#define CONFIG_SUBQ	  1	/* Subq to send bind requests to	*/
#define ABORT_SUBQ	  1	/* Subq to send abort requests to	*/
#define EVENT_SUBQ	  2	/* Subqueue for lower managers to	*/
				/*   post event messages to		*/
#define REPLY_SUBQ	  3	/* Subqueue for lower managers to	*/
				/*   post reply messages to		*/
#define N_SUBQS	 4		/* The number of subqueues used: If use sub-
				     queue 0 through N, N_SUBQS = (N + 1). */

/*
N_MSGS is the maximum number of messages the manager has outstanding at any one
time.  Unfortunately,  HP-UX does not really care about this number, so we will
set it to 2, which should be correct.  The only time we	 have  two  outstanding
messages  is  during  binding,	when  we reply to the configurator, and send an
identify request, and when we are aborting a request.
*/
#define N_MSGS	2

/* Following are the memory residency and execution options */
#define CREATE_OPTIONS ELEMENT_OF_32(MGR_CODE_RESIDENT)	 |\
		       ELEMENT_OF_32(MGR_DATA_RESIDENT)	 |\
		       ELEMENT_OF_32(MGR_POOL_RESIDENT)	 |\
		       ELEMENT_OF_32(MGR_ICS_EXECUTABLE) |\
		       ELEMENT_OF_32(MGR_PROC_EXECUTABLE)

#define IDENTLEN	9	/* number of bytes to read from the card 
				   identifier */
#define RTICARDID	9	/* ID expected, indicating RTI is present */
/* due to hardware, ask for an extra word in CIO.
   NIO hardware uses correct # of words */
#define PREFIXQWSIZE	((pda->Q_size*PREFIXSIZE)/2 + pda->xtracio)

#define DMATIMEOUT	(4 * 1000 * 1000)	/* 4 seconds */
#define DMAPFAILTIMEOUT	(30 * 1000 * 1000)	/* 60 seconds */
#define DMADIAGTIMEOUT	(10 * 1000 * 1000)	/* 10 seconds */
#define DESTDIAGADDR	0xFC000	/* address of destructive diagnostics */
#define PROBEBUFSIZE	(2 * 1024)	/* desired size for probe write buf */
/************************************************************************/


/**************************** EXTERNAL DATA *****************************/
/* The following will be used to access a pda given an LU number. */
extern rti0_pda_type	*rti0_pda_map[]; /* lu==>pda mapping table	*/

/* ANALYZE END. This is here in order to severe the file for analyze */
