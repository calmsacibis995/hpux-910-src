/*
 * @(#)conf.h: $Revision: 1.26.83.4 $ $Date: 93/09/17 18:24:06 $
 * $Locker:  $
 */

#ifndef _SYS_CONF_INCLUDED /* allows multiple inclusion */
#define _SYS_CONF_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */

#ifdef __hp9000s800
#ifdef _KERNEL_BUILD
#include "../machine/iodc.h"
#else /* ! _KERNEL_BUILD */
#include <machine/iodc.h>
#endif /* _KERNEL_BUILD */
#endif /* __hp9000s800 */

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 */
struct bdevsw
{
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_strategy)();
	int	(*d_dump)();
	int	(*d_psize)();
	int	d_flags;
	int	(*d_mount)();
};

#define         C_ALLCLOSES     01     /* call device close on all closes */
#define         C_NODELAY       02     /* if no write delay on block devices */
#define         C_EVERYCLOSE    04     /* call device close on all closes */
#define		MGR_IS_MP	010	/* identifies that the device driver
					 * is MP-safe */
#define 	C_CLONESMAJOR   020	/* driver clones maj num (and min) */

extern	int brmtdev;			/* the remote device pseudo-driver */

#ifdef _KERNEL
#ifdef	__hp9000s800
extern struct	bdevsw bdevsw[];
#else	/* not __hp9000s800 */
struct	bdevsw bdevsw[];
#endif	/* else not __hp9000s800 */
#endif

/*
 * Character device switch.
 */
struct cdevsw
{
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_ioctl)();
	int	(*d_select)();
/* CONCERN diffs between 200 and __hp9000s800. */
#ifdef __hp9000s800
	int	(*d_option1)();
#endif /* __hp9000s800  */
	int     d_flags;
/* MGR_IS_MP	is valid for this field */
};

extern	int crmtdev;			/* the remote device pseudo-driver */

#ifdef _KERNEL
#ifdef	__hp9000s800
extern struct	cdevsw cdevsw[];
#else	/* not __hp9000s800 */
struct	cdevsw cdevsw[];
#endif	/* else not __hp9000s800 */
#endif

#ifdef __hp9000s800

#define	FMNAMESZ	8

struct fmodsw {
	char	f_name[FMNAMESZ+1];
	struct  streamtab *f_str;	/* HP-UX NSE stream table */
};

extern struct fmodsw fmodsw[];

extern int	fmodcnt;
#endif /* __hp9000s800 */

#ifdef __hp9000s800
/*
 * tty line control switch.
 */
struct linesw
{
	int	(*l_open)();
	int	(*l_close)();
	int	(*l_read)();
	int	(*l_write)();
	int	(*l_ioctl)();
	int	(*l_select)();
	int	(*l_rint)();
	int	(*l_xint)();
	int	(*l_start)();
	int	(*l_control)();
};
#else /* not __hp9000s800 */
/*
 * tty line control switch.
 */
struct linesw
{
	int	(*l_open)();
	int	(*l_close)();
	int	(*l_read)();
	int	(*l_write)();
	int	(*l_ioctl)();
#define l_input l_rint
	int	(*l_rint)();
	int	(*l_rend)();
	int	(*l_meta)();
	int	(*l_start)();
	int	(*l_modem)();
};
#endif	/* else not __hp9000s800 */

#ifdef _KERNEL
#ifdef	__hp9000s800
extern struct	linesw linesw[];
#else	/* not __hp9000s800 */
struct	linesw linesw[];
#endif	/* else not __hp9000s800 */
#endif

/*
 * Swap device information
 */
#ifdef __hp9000s800
#define MAXNSWDEV	8 /*maximal number of swap devices per site*/
#endif
/*
 * Swap device information
 */
typedef struct swdevt
{
        dev_t   sw_dev;         /* swap device          */
        int     sw_enable;      /* enabled              */
        int     sw_start;       /* offset for 300/700   */
        int     sw_nblks;       /* number of blocks     */
        int     sw_nfpgs;       /* # of free pages      */
        int     sw_priority;    /* priority of device   */
        int     sw_head;        /* first swaptab[] entry*/
        int     sw_tail;        /* last swaptab[] entry */
        struct swdevt *sw_next; /* next swap device     */
} swdev_t;
#ifdef _KERNEL
#ifdef	__hp9000s800
extern struct	swdevt swdevt[];
#else	/* not __hp9000s800 */
struct	swdevt swdevt[];
#endif	/* else not __hp9000s800 */
#endif

#ifndef  _WSIO 	/* multi-dump is only for 800 */
#define MAXDUMPDEV       32

/*
 * Dump device information
 */
typedef struct dumpdevt
{
        dev_t   dp_dev;         /* dump device                  */
        int     used;           /* used or not                  */
        int     index;          /* index to disc_par            */ 
	int     size;           /* size of dp_dev               */
        int     index_to_kdt;   /* index to the kern_dev_tab    */
        int     dumplo;         /* dump location                */
        int     initialized;    /*  initialize flag             */
        dev_t   lv;             /* corresponding LV	 	*/
        struct dev_entry  ddev_entry; /* contains module path, hpa, spa */
} dumpdev_t;

extern struct   dumpdevt  dumpdevt[];

#endif  /* ! _WSIO */


#ifdef __hp9000s800
/* subsystem data structure - the only link of the main kernel with
   a subsystem.  This structure contains the init routines of each
   subsystem, and is initialized in conf.c (generated by uxgen).
*/

struct subsys_mgr_type {
	int	(*init)();
};


#define MAX_SUBSYS	16

typedef char			subsys_name_type[MAX_SUBSYS];
typedef subsys_name_type	subsys_names_type[];
extern subsys_name_type		*p_subsys_names;

/* filesystem data structure - the only link of the main kernel with
   a filesystem.  This structure contains the init routines of each
   filesystem, and is initialized in conf.c (generated by uxgen).
*/

struct filesys_mgr_type {
	int	(*init)();
};


#define MAX_FILESYS	16

typedef char			filesys_name_type[MAX_FILESYS];
typedef filesys_name_type	filesys_names_type[];
extern filesys_name_type	*p_filesys_names;
#endif /* __hp9000s800 */

#endif /* _SYS_CONF_INCLUDED */
