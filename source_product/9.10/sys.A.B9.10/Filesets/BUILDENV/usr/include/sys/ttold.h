/* @(#) $Revision: 1.14.83.5 $ */    
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/ttold.h,v $
 * $Revision: 1.14.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/08 18:24:26 $
 */
#ifndef _SYS_TTOLD_INCLUDED /* allows multiple inclusion */
#define _SYS_TTOLD_INCLUDED

struct	sgttyb {
	char	sg_ispeed;
	char	sg_ospeed;
	char	sg_erase;
	char	sg_kill;
	int	sg_flags;
};

/* modes */
#define	O_HUPCL	01
#define	O_XTABS	02
#define	O_LCASE	04
#define	O_ECHO	010
#define	O_CRMOD	020
#define	O_RAW	040
#define	O_ODDP	0100
#define	O_EVENP	0200
#define	O_NLDELAY	001400
#define	O_NL1	000400
#define	O_NL2	001000
#define	O_TBDELAY	002000
#define	O_NOAL	004000
#define	O_CRDELAY	030000
#define	O_CR1	010000
#define	O_CR2	020000
#define	O_VTDELAY	040000
#define	O_BSDELAY	0100000

#define	TIOCGETP	_IOR('t', 8,struct sgttyb)/* get parameters -- gtty */
#define	TIOCSETP	_IOW('t', 9,struct sgttyb)/* set parameters -- stty */


#endif /* _SYS_TTOLD_INCLUDED */
