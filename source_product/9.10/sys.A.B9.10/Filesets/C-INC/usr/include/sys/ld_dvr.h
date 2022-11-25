/*
 * @(#)ld_dvr.h: $Revision: 1.19.83.4 $ $Date: 93/09/17 18:28:15 $
 * $Locker:  $
 *
 */

#ifndef _SYS_LD_DVR_INCLUDED /* allows multiple inclusion */
#define _SYS_LD_DVR_INCLUDED

/*----------------------------------------------------------------------
 | The following are the defines for the input status bits that are
 | passed to the line discipline in the receive interrupt procedure.
 ----------------------------------------------------------------------*/
#ifdef __hp9000s800
#define	LDR_PARITY	0x01
#define	LDR_OVERRUN	0x02
#define	LDR_FRAMING	0x04
#define	LDR_BREAK	0x08
#define	LDR_MODEM	0x10
#define	LDR_OVERFLOW	0x20
#define	LDR_INTERNAL	0x40

#define	OVERRUN		LDR_OVERRUN
#define	FRERROR		LDR_FRAMING
#define	PERROR		LDR_PARITY
#define	L_BREAK		LDR_BREAK

/*----------------------------------------------------------------------
 | The following are the defines for the control bits returned to the
 | driver by the transmit interrupt procedure.
 ----------------------------------------------------------------------*/

#define	LDX_TBUF	0x00010000
#define	LDX_DLYTIME	0x00020000
#define	LDX_DLYCHAR	0x00040000

#define	CPRES		LDX_TBUF

#endif /* __hp9000s800 */
/*----------------------------------------------------------------------
 | The following is the enumeration for the discipline CONTROL function
 | codes.
 ----------------------------------------------------------------------*/

enum	ldc_func	{ LDC_PRELIM,	LDC_LCHANGE,	LDC_LOPEN,
			  LDC_LCLOSE,	LDC_CSTAT,	LDC_INIT,
			  LDC_PUTC,	LDC_BROKEN,	LDC_WAIT,
			  LDC_FLUSH,	LDC_BCHANGE,	LDC_IOCTL,
			  LDC_RFLUSH,	LDC_WFLUSH,	LDC_DFLOW,
			  LDC_VHANGUP,
			};

/*----------------------------------------------------------------------
 | The following is the enumeration for the driver CONTROL function
 | codes.
 ----------------------------------------------------------------------*/

enum	sdc_func	{ SDC_OUTPUT,	SDC_RESUME,	SDC_RNOTIFY,
			  SDC_SUSPEND,	SDC_SNOTIFY,	SDC_WFLUSH,
			  SDC_RFLUSH,	SDC_SETSTOPI,	SDC_SETSTOPO,
			  SDC_UNBLOCK,	SDC_BLOCK,	SDC_BREAKON,
			  SDC_BREAKOFF,	SDC_WAIT,	SDC_PUTC,
			  SDC_RESTORE,	SDC_BROKEN,	SDC_GETC,
			  SDC_PUTB,	SDC_GETB,	SDC_CALLOUT,
#ifdef __hp9000s800
			  SDC_INPUT,    SDC_TTY,
#endif /* __hp9000s800 */
			  SDC_PARM,	SDC_MODEM_CNTL, SDC_MODEM_STAT,
			  SDC_VHANGUP,
			};
#ifdef __hp9000s800
#define	T_OUTPUT	SDC_OUTPUT
#define	T_RESUME	SDC_RESUME
#define	T_RNOTIFY	SDC_RNOTIFY
#define	T_SUSPEND	SDC_SUSPEND
#define	T_SNOTIFY	SDC_SNOTIFY
#define	T_WFLUSH	SDC_WFLUSH
#define	T_RFLUSH	SDC_RFLUSH
#define	T_SETSTOPI	SDC_SETSTOPI
#define	T_SETSTOPO	SDC_SETSTOPO
#define	T_UNBLOCK	SDC_UNBLOCK
#define	T_BLOCK		SDC_BLOCK
#define	T_BREAK		SDC_BREAKON
#define	T_TIME		SDC_BREAKOFF
#define	T_WAIT		SDC_WAIT
#define	T_PUTC		SDC_PUTC
#define	T_RESTORE	SDC_RESTORE
#define	T_INPUT		SDC_INPUT
#define	T_TTY		SDC_TTY
#define	T_PARM		SDC_PARM
#define	T_MODEM_CNTL	SDC_MODEM_CNTL
#define	T_MODEM_STAT	SDC_MODEM_STAT

/*----------------------------------------------------------------------
 | The following are defines for the "data" parameter to mux*_control.
 ----------------------------------------------------------------------*/
#define FORCE_DRAIN	1

#endif /* __hp9000s800 */

#endif /* not  _SYS_LD_DVR_INCLUDED */
