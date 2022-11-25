/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/vmmac.h,v $
 * $Revision: 1.45.83.4 $       $Author: kcs $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 18:38:58 $
 */
#ifndef _SYS_VMMAC_INCLUDED /* allows multiple inclusion */
#define _SYS_VMMAC_INCLUDED

/* Current text|data|stack size (in pages) for the specified process.
 *
 * Note that the stack size is for the user stack only; it does not
 * include the uarea or kernel stack.
 *
 */
#define TEXTSIZE(p)	(textsize(p))
#define DATASIZE(p)	(datasize(p))
#define USTACKSIZE(p)	(ustacksize(p))

/* Bytes to pages without rounding, and back */
#define	btop(x)		(((unsigned)(x)) >> PGSHIFT)
#define btohp(x)	(((unsigned)(x)) >> HPGSHIFT)
#define	ptob(x)		((unsigned)((x) << PGSHIFT))
#define hptob(x)	((unsigned)((x) << HPGSHIFT))
#define hptop(x)	btop(hptob(x))
#define pagedown(ADDR) ((u_int)(ADDR) &~ (NBPG - 1))

#define pageup(ADDR) ((u_int)((u_int)(ADDR) + (NBPG - 1)) &~ (NBPG - 1))

/* Pages to disc blocks with rounding and back */

#define ptod(x) (x<<(PGSHIFT- DEV_BSHIFT))
#define dtop(x) (x>>(PGSHIFT- DEV_BSHIFT))

/* Given an addr and length determing the number of pages spanned. */

#define pagesinrange(ADDR, BYTES) \
	((pageup(((u_int)(ADDR))+(BYTES)) - pagedown((u_int)(ADDR))) >> PGSHIFT)


#ifdef __hp9000s800
#define pgndx_sext(ndx) \
	(((ndx) & (1<<(PGNDXWDTH-1))) ?	((ndx) | (~((1<<PGNDXWDTH)-1))): (ndx))
#define iobtop(x)	((int)(pgndx_sext(btop(x))))
#if NBPG == 2048
#define sign21ext pgndx_sext /* for backward compatibility */
#endif /* NBPG == 2048 */
#endif /* hp9000s800 */

#define	btorp(x)	(((unsigned)(x) + NBPG -1) >> PGSHIFT)
#define btorhp(x)	(((unsigned)(x) + NBHPG -1) >> HPGSHIFT)

#define POFFMASK        (NBPG-1)
#define poff(X)   ((uint)(X) & POFFMASK)		/* page offset */

/* 
 * You find a process's Uarea by dereferencing through
 * p_upregion.
 */
#define	pcbb(p)		\
	(p->p_upregion->p_space), (p->p_upregion->p_vaddr)

/* Average new into old with aging factor time */
#define	ave(smooth, cnt, time) \
	smooth = ((time - 1) * (smooth) + (cnt)) / (time)

#ifdef __hp9000s300
	/* XXX VANDYS this right for cluster == page only; compatibility! */
#define anycl(pte, fld) (pte->fld)
#endif /* __hp9000s300 */

/*
 * approximations of a process' resident set sizes.
 */
 /* must hold the sched_lock to use any of these macros */

#define PRSSIZE_APPROX(p)			\
	((p->p_vas && !(p->p_flag & SWEXIT)) ?				\
		((p->p_flag & SLOAD) ?		\
	 		(p->p_vas->va_prss) :	\
	 		(p->p_vas->va_swprss)	\
		) :				\
		(0)				\
	)

#define RSS_P_APPROX(p)				\
	((p->p_vas && !(p->p_flag & SWEXIT)) ?				\
		(p->p_vas->va_rss) :		\
		(0)				\
	)

#define PRSS_P_APPROX(p)			\
	((p->p_vas && !(p->p_flag & SWEXIT)) ?				\
		(p->p_vas->va_prss) :		\
		(0)				\
	)

#endif /* _SYS_VMMAC_INCLUDED */
