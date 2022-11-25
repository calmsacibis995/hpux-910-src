/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/pt.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:16:59 $
 */
/* HPUX_ID: @(#)pt.h	52.2		88/04/26 */
/* @(#) $Revision: 1.3.84.3 $ */       
#ifndef _MACHINE_PT_INCLUDED /* allow multiple inclusions */
#define _MACHINE_PT_INCLUDED

#ifdef __hp9000s300
/* misc. constants */
#define		PT_SIZE	128


/* storage local to driver */
struct PT_stat {
	int		p_cnt; 	   /* # of chars in the buffer when writing */
	struct buf	p_bufbuf;
	char            p_buffer[PT_SIZE];	
};
#endif /* __hp9000s300 */
#endif /* _MACHINE_PT_INCLUDED */
