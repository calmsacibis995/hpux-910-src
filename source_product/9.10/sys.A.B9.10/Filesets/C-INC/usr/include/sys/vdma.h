/*
 * @(#)vdma.h: $Revision: 1.6.83.4 $ $Date: 93/09/17 18:38:29 $
 * $Locker:  $
 */

/*
 * info for notification on VDMA pages
 *
 */

#ifndef _VDMA_INCLUDED
#define _VDMA_INCLUDED

#define _MAX_VDMA_DEVICES 8

struct vnotify {
	int	v_type;
	vas_t 	*v_vas;
	space_t v_space;
	caddr_t v_vaddr;
	int	v_cnt;
	int	v_hil_pfn;
	int	v_flags;
};

/* defines for type */

#define VDMA_GETBITS		0

#define	VDMA_DELETETRANS	1
#define	VDMA_USERPROTECT	2
#define	VDMA_USERUNPROTECT	3
#define	VDMA_PROCDETACH		4
#define VDMA_UNVIRTUALIZE	5
#define VDMA_UNSETBITS		6
#define VDMA_READONLY           7

/* for flags word */
#define NOTIFY_RW 0
#define NOTIFY_RO 1

#endif /* _VDMA_INCLUDED */
