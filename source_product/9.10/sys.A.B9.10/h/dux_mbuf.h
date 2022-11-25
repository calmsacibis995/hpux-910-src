/*
 * @(#)dux_mbuf.h: $Revision: 1.7.83.4 $ $Date: 93/09/17 18:25:47 $
 * $Locker:  $
 */

#ifndef _SYS_DUX_MBUF_INCLUDED /* allow multiple inclusions */
#define _SYS_DUX_MBUF_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../h/mbuf.h"
#else /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */

/*
** dux_mbuf declaration
**
*/

#define	dux_mbuf	mbuf

/*
** dux_mbuf head, to typed data
*/
#define	dux_mtod(x,t)	((t)((int)(x) + (x)->m_off))

/*
** Mbuf statistics.
*/
struct dux_mbstat {
	short	m_netbufs;		/* buffers in netbuf pool         */
	short	m_nbfree;		/* buffers in netbuf free list    */
	u_int	m_nbtotal;		/* Running total of netbufs used  */
	short	m_netpages;		/* Pages allocated to net pool	  */
	short	m_freepages;		/* Pages in free list		  */
	u_int	m_totalpages;		/* Running total of pages used    */
};

#ifdef _KERNEL
struct	dux_mbstat dux_mbstat;
#endif /* _KERNEL */
#endif /* _SYS_DUX_MBUF_INCLUDED */
