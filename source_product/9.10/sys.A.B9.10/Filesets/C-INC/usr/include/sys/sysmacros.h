/* @(#) $Revision: 1.21.83.6 $ */
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/sysmacros.h,v $
 * $Revision: 1.21.83.6 $        $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/11/14 11:10:47 $
 */
#ifndef _SYS_SYSMACROS_INCLUDED /* allows multiple inclusion */
#define _SYS_SYSMACROS_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

/*
 * Some macros for units conversion
 */

/*
 * Macros for handling inode numbers:
 *     inode number to file system block offset.
 *     inode number to cylinder group number.
 *     inode number to file system block address.
 */
#define itoo(fs,x)	((x) % INOPB(fs))
#define itog(fs,x)	((x) / (fs) -> fs_ipg)
#define itod(fs,x)	\
	((daddr_t)(cgimin(fs, itog(fs, x)) + \
	(blkstofrags((fs), (((x) % (fs) -> fs_ipg) / INOPB(fs))))))

#ifdef __hp9000s300
/* page count to physical page frame number */
#define btoc(x) (((unsigned)(x)+(NBPG -1))>>PGSHIFT)	/* bytes to clicks */
#define pctopfn(x)	((x) + physmembase)
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#define btow(x) ((unsigned)(x) >> BYTESHIFT)		/* bytes to word */
#endif /* __hp9000s800 */

/* major part of a device */
#define major(x)	((long)(((unsigned)(x)>>24)&0xff)) /* 8 bit major */
#define bmajor(x)	major(x)
#define brdev(x)	(x)

/* minor part of a device */
#define minor(x)	((long)((x)&0xffffff))		/* 24 bit minor */

#define MINOR_FORMAT	"0x%06.6x"

/* make a device number */
#define makedev(x,y)	((dev_t)(((x)<<24) | (y & 0xffffff)))

/* works for both s300 and s700 */
#ifdef _WSIO
#define m_selcode(x)	(int)((unsigned)(x)>>16&0xff)
#define m_unit(x)	(int)((unsigned)(x)>>4&0xf)
#define m_volume(x)	(int)((unsigned)(x)&0xf)
#define M_DEVMASK	(0xffff00)
#endif /* _WSIO */

#ifdef __hp9000s300
/* make a minor number */
#define makeminor(sc, ba, un, vl)	((long)((sc)<<16|(ba)<<8|(un)<<4|vl))

/* common field definitions within the minor */
#define m_busaddr(x)	(int)((unsigned)(x)>>8&0xff)
#define m_flags(x)	(int)((unsigned)(x)&0xff)
#define m_port(x)	(int)((unsigned)(x)>>8&0xff)
#endif /* __hp9000s300 */

#if defined(__hp9000s800) && defined(_WSIO)

/* make a minor number from the select code, function number, and driver bits */
#define makeminor(sc, f_num, drvr) ((long)((sc)<<16|(f_num)<<12|(drvr)))

/* common field definitions within the minor */
#define m_vsc(x)	(int)((unsigned)(x)>>20&0x0f)
#define m_slot(x)	(int)((unsigned)(x)>>16&0xf)
#define m_funcnum(x)	(int)((unsigned)(x)>>12&0xf)
#define m_busaddr(x)	(int)((unsigned)(x)>>8&0xf)	/* user bits */

#endif /* __hp9000s800 && _WSIO */

#ifdef _KERNEL_BUILD
#define toupper(x) (((x) >= 'a' && (x) <= 'z') ? (x-32) : (x))
#endif

#endif /* _SYS_SYSMACROS_INCLUDED */

