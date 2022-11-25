/*
 * @(#)hil.h: $Revision: 1.12.83.4 $ $Date: 93/12/09 15:24:45 $
 * $Locker:  $
 */
#ifndef _GRAF_HIL_INCLUDED /* allows multiple inclusion */
#define _GRAF_HIL_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _KERNEL
#   include "../h/tty.h"				 /* for struct clist */
#ifdef _WSIO
#   ifndef __hp9000s300
#   include "../wsio/timeout.h"			 /* for struct sw_intloc */
#   endif
#endif /* _WSIO */

    /*
     * HIL and HILKBD can use the same code for read and select, so these
     * defines ensure this.
     * conf.c includes this file by including ../machine/space.h.
     */
#   define hilkbd_read		hil_read
#   define hilkbd_select	hil_select

#   ifndef TRUE
#	define TRUE	1
#	define FALSE	0
#   endif

#   define HIL_SUCCESS	0
#   define HIL_FAILURE	1
#   define HIL_PFAIL	2
#endif /* _KERNEL */

/*
 * Values for g->command_device and g->active_device
 */
#define HIL_DEVICE_UNDEFINED	0
    /* regular HIL devices	1-7 */
#define HIL_DEVICE_COOKED_KBD	8
#define HIL_DEVICE_COOKED_RPG	9

/*
 * 8042 status register masks
 */
#define DATA_RDY        0x01
#define CMD_PEND        0x02

/*
 * HIL interrupt masks
 */
#define HILOFF		0x5D	/* mask off all interrupts except reset */
#define HILON		0x5C	/* mask off all ints but HIL & RESET */

#define KEYSHIFT	0x10	/* key was shifted  */
#define KEYCNTL		0x20	/* key was control key */

/*
 * Status 5 Bits
 */
#define	H_LERROR	0x080		/* Status 5 Error Bit */
#define	H_LRECONFIG	0x080		/* Loop has reconfigured */
#define	H_LDERROR	0x081		/* Data error to Loop */
#define	H_TIMEOUT	0x082		/* Loop Timeout */
#define	H_LDOWN		0x084		/* Loop is being reconfigured */
#define	H_LCOMMAND	0x008		/* Data to follow is the Command
					   which caused the previous data */
#define	H_POLLING	0x010		/* Data to follow is in response
					   to a poll command */
#define	H_DEVADR	0x007		/* Device address */
#define	H_LMAXDEVICES	9		/* Maximum number of loop devices
					   plus a device for cooked keys
					   and RPG input */
#ifdef _WSIO
#   define H_NIMITZKBD	8		/* Address of 'cooked' keyboards */
#   define H_NIMITZKNOB	9		/* Address of 'cooked' knob */
#else
#   define H_HILKBDKBD	8		/* Address of 'cooked' keyboards */
#   define H_HILKBDKNOB	9		/* Address of 'cooked' knob */
#endif
#define H_MAXHILS	16		/* Maximum number of loops supported */
#define MAX_PACKET_LENGTH 20		/* Maximum poll packet length including
					   the time stamp */

/*
 * LPCTRL Bits
 */
#define	H_DORECONFIGURE	0x080		/* Reconfigure the Loop */
#define H_COOKED	0x010		/* Cook selected keyboards */
#define	H_NORECONFIG	0x004		/* Don't Report Loop Reconfiguration */
#define	H_NOERRS	0x002		/* Don't Report Loop Errors */
#define	H_DOAUTOPOLL	0x001		/* Autopoll */

/*
 * Commands
 */
#define H_BELLCMD	0xc4
#define H_CLEARNMI	0xb2
#define H_GETMODIFIERS  0x05
#define H_IDCMD		0x03
#define H_INTON		0x5c
#define H_KBDSTATUS	0x05
#define H_OLDBELL	0xa3
#define H_READBUSY	0x02
#define H_READCONFIG	0x11
#define H_READCTRL	0xfb
#define H_READKBDS 	0xf9
#define H_READLANG	0x12
#define H_READLSTAT	0xfa
#define H_READTIME	0x13
#define H_REP_DELAY	0xa0
#define H_REP_RATE	0xa2
#define H_RPG_RATE	0xa6
#define H_STARTCMD	0xe0
#define H_TIMEOUT20	0xfe	/*  20 milliseconds  	*/
#define H_TIMEOUT200    0xec	/*  200 milliseconds  	*/
#define H_TRIGGER	0xc5
#define H_WRITECTRL	0xeb
#define H_WRITEDATA	0xe0
#define H_WRITEKBDS	0xe9

/*
 * HIL commands.
 */
#define H_HIL_PST	0x05

#define	H_DATASTARTING	0
#define	H_DATAENDED	1
#define	H_RESETDEVICE	2
#define H_INT_MASK	0x1f

#define NMI_FASTHANDSHAKE	0x4

#define H_IOC_RIDC      "H"
#define H_IOC_HIDC      "h"
#define H_IOC_NIDC      "N"
#define H_IOCSIZE	0x3fff0000
#define H_IOCCMD	0xff

/*
 * dev fields for the 8042.
 */
#define H_LU_MASK	0x0f<<8
#define H_HIL_DEVICE	0x0f<<4
#define DEV_TO_LU(dev)		(((dev) & H_LU_MASK) >> 8)
#define DEV_TO_LOOPDEV(dev)	(((dev) & H_HIL_DEVICE) >> 4)

#define HILHOG		512
#define HILIPRI		PZERO+3
#define HILBUF_SIZE	384

#ifdef _KERNEL
    /*
     * Device data record.
     */
#   ifdef _WSIO
    /*
     * HIL buffer record.
     */
    struct hilbuf {
	ushort	 *g_inbuf;	/* Input queue */
	ushort	 *g_in_head;	/* Head of input queue */
	ushort	 *g_in_tail;	/* Tail of input queue */
	ushort	  g_in_count;	/* Number of characters in queue */
	unsigned char g_int_flag;	/* Interrupt flags */
	unsigned char g_rcv_flag;	/* Pending interrupt for receive */
	int		  g_int_lvl;	/* Interrupt level of the HIL */
	struct sw_intloc g_rcv_intloc;	/* Software trigger receive
							    structure */
    };
#   endif /* _WSIO */

    /*
     * Structure for the memory mapped registers in the 8042
     */
    struct data8042 {
#   ifdef _WSIO
#       ifdef __hp9000s300
	    unsigned char dummy1;		/* 0 */
	    unsigned char id;			/* 1: id register */
	    unsigned char dummy2;		/* 2 */
	    unsigned char interrupt;		/* 3: interrupt register */
	    unsigned char dummy3[0x8000-4+1];	/* 4 */
	    unsigned char hil_data;		/* 8001: data */
	    unsigned char dummy4;		/* 8002 */
	    unsigned char cmd_stat;		/* 8003: status */
#	else /* 700 */
	    unsigned char dummy[0x800];	/* 0 */
	    unsigned char hil_data;		/* 800: data */
	    unsigned char cmd_stat;		/* 801: status */
#	endif /* __hp9000s300 */
#   else /* SIO, 800 */
	    unsigned int hil_data;
	    unsigned int hil_fill;
	    unsigned int cmd_stat;
#   endif /* _WSIO */
    };

    struct h_ioctl {
	unsigned ioc_in      : 1;
	unsigned ioc_out     : 1;
	unsigned ioc_size    : 14;
	unsigned ioc_idc     : 8;
	unsigned ioc_cmd     : 8;
    };

#   ifdef __hp9000s800
	/*
	 * HIL and HILKBD can use the same code for read and select, so these
	 * defines ensure this.
	 * conf.c includes this file by including ../machine/space.h.
	 */
#	define hilkbd_read	hil_read
#	define hilkbd_select	hil_select
#   endif /* __hp9000s800 */

    extern struct hilrec *hil_loop[];

#   ifdef _WSIO
	extern int hil_buffer(), hil_service(), hil_isr();
	extern struct hilrec *which_hil();
	extern int hil_cmd(), hil_ldcmd();
	extern int num_hil;				     /* from space.h */
#   endif /* _WSIO */

#endif /* _KERNEL */

/*
 * Structure for select().
 */
struct hilselect {
    struct proc *hil_selp;
    short hil_selflag;
};
#define HSEL_COLL	1	/* select() collision flag */

struct devicerec {
    struct clist	hdevqueue;
    struct hilselect	hil_sel;
    unsigned char	open_flag;
    unsigned char	sleep;
};

#ifdef __hp9000s300
    /*
     * This structure is for the DIO HIL card.  It has addresses as shown,
     * and can be moved about DIO space via switches.
     * The built-in HIL (as on the big cheese or human interface card)
     * has data at 0x428001, and status at 0x428003.  We pretend that
     * that card starts at 0x420000, and do not use "id" or "interrupt",
     * since they do not apply to the built-in HIL.  We detect the built-in
     * HIL by its h->sc of 0, since it does not realy have a select code.
     */
#endif
/*
 * HIL data record
 */
struct hilrec {
    struct data8042    *hilbase;
    int			unit;
    unsigned char	has_ite_kbd;
    unsigned char	hilkbd_took_ite_kbd;
    unsigned char	active_device;
    unsigned char	command_device;
    unsigned char	command_ending;
    unsigned char	packet[MAX_PACKET_LENGTH];
    unsigned char	packet_length;
    unsigned char	loop_response[MAX_PACKET_LENGTH];
    unsigned char	response_length;
    unsigned char	cooked_mask;
    unsigned char	current_mask;
    unsigned char	num_ldevices;
    unsigned char	repeat_rate;
#   ifdef _WSIO
	unsigned char	loopcmddone;
	unsigned char	collecting;
	unsigned char	raw_8042_data;
	unsigned char	has_8042_data;
	int		sc;		      /* select code, 0 for internal */
	struct hilbuf	gb;
#   else
	unsigned char	 loopbusy;
	unsigned char	 command_in_progress;
	unsigned char	 waiting_for_loop;
	unsigned char	 repeat_delay;
	int		 (*graph_service)();
#   endif
    struct devicerec	 loopdevices[H_LMAXDEVICES+1];
    /* FIXME. */
    unsigned int raw_8042_status;
};

#ifndef _WSIO
#   ifdef _KERNEL
#	include "../sio/llio.h"
#   else
#	define port_num_type int
#   endif /* _KERNEL */

    typedef enum {
	HIL_PORT_UNINITIALIZED, HIL_PORT_BOUND, HIL_PORT_NORMAL
    } hil_state;

    typedef struct {
	hil_state	  state;
	int		 *hilrec;    /* cast to "struct iterminal" if needed */
	port_num_type	  my_port, parent_port;
	int		  mgr_index, msg_id;
    } hil_pda_type;

#   define HIL_EVENT_SUBQ	3
#   define HIL_REPLY_SUBQ	4
#   define NUM_HIL_SUBQS	5

#   define which_hil(dev)	(&hil_loop[(DEV_TO_LU((dev))])
#   define which_lu(g, dev)	(DEV_TO_LU((dev)))
#   define MAX_HIL0		4
#endif /* ndef _WSIO */

#define HIL_FROM_HIL	0

#endif /* ! _GRAF_HIL_INCLUDED */
