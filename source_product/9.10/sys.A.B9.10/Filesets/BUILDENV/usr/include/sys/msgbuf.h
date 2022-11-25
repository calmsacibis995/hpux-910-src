/* @(#) $Revision: 1.12.83.5 $ */       
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/msgbuf.h,v $
 * $Revision: 1.12.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 11:48:46 $
 */
#ifndef _SYS_MSGBUF_INCLUDED /* allows multiple inclusion */
#define _SYS_MSGBUF_INCLUDED

#define	MSG_MAGIC	0x063060
#define	MSG_BSIZE	(4096 - 2 * sizeof (long))
struct	msgbuf {
	long	msg_magic;
	long	msg_bufx;
	char	msg_bufc[MSG_BSIZE];
};
#ifdef	_KERNEL
#ifdef	__hp9000s800
extern	struct	msgbuf msgbuf;
#else	/* not __hp9000s800 */
#ifdef	__hp9000s300
struct	msgbuf Msgbuf;
#else	/* not __hp9000s300 */
struct	msgbuf msgbuf;
#endif	/* else not __hp9000s300 */
#endif	/* else not __hp9000s800 */
#endif /* _KERNEL */

#endif /* _SYS_MSGBUF_INCLUDED */
