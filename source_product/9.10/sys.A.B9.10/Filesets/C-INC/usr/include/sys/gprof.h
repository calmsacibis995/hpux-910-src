/* @(#)gprof.h: $Revision: 1.5.84.3 $ $Date: 93/09/17 21:01:15 $ */

/* @(#) $Revision: 1.5.84.3 $ */    
struct phdr {
    char	*lpc;
    char	*hpc;
    int		ncnt;
#ifdef	notdef
    int		idle_count;
    int		user_code_count;
    int		count_overflows;
#endif /* hp9000s300 */
};

    /*
     *	histogram counters are unsigned shorts (according to the kernel).
     */
#define	HISTCOUNTER	unsigned short

    /*
     *	fraction of text space to allocate for histogram counters
     *	here, 1/2
     */
#define	HISTFRACTION	2

#ifdef	hp9000s800
#ifdef OLDCALL
#define	HASHFRACTION    0xa
#else
#define HASHFRACTION    0x2
#endif
#endif	/* hp9000s800 */

#ifdef __hp9000s300
#define HASHFRACTION	1
#endif


/* tunables */ /* XXX */
extern    histfrac;
extern    hashfrac;


#ifdef	hp9000s800

    /*
     *	percent of text space to allocate for tostructs
     *	with a minimum.
     */
#define ARCDENSITY	2
#define MINARCS		50
#else
#define ARCDENSITY	5
#define MINARCS		50
#endif

struct tostruct {
    char		*selfpc;
    long		count;
    unsigned short	link;
    unsigned char	procindex;	/* Processor index */
    unsigned char	spare;
};

    /*
     *	a raw arc,
     *	    with pointers to the calling site and the called site
     *	    and a count.
     */
struct rawarc {
    unsigned long	raw_frompc;
    unsigned long	raw_selfpc;
    long		raw_count;
};

    /*
     *	general rounding functions.
     */
#define ROUNDDOWN(x,y)	(((x)/(y))*(y))
#define ROUNDUP(x,y)	((((x)+(y)-1)/(y))*(y))

#ifdef	__hp9000s300
    /* operation parameters for kern_prof intrinsic */
#define GPROF_INIT	0
#define GPROF_ON	1
#define GPROF_OFF	2
#define GPROF_READ_HDR	3
#define GPROF_READ_DATA	4
#endif	/* __hp9000s300 */
