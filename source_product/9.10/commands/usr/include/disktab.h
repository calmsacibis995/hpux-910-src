/* @(#) $Revision: 66.3 $ */     

#ifndef _DISKTAB_INCLUDED /* allow for multiple inclusions */
#define _DISKTAB_INCLUDED

#ifdef HFS
#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Disk description table, see disktab(5)
 */
#ifdef _INCLUDE_HPUX_SOURCE

#define	DISKTAB		"/etc/disktab"

#define NSECTIONS	16

struct	disktab {
	char	*d_name;		/* drive name */
	char	*d_type;		/* drive type */
	int	d_secsize;		/* sector size in bytes */
	int	d_ntracks;		/* # tracks/cylinder */
	int	d_nsectors;		/* # sectors/track */
	int	d_ncylinders;		/* # cylinders */
	int	d_rpm;			/* revolutions/minute */
	struct	partition {
		int	p_size;		/* #sectors in partition */
		short	p_bsize;	/* block size in bytes */
		short	p_fsize;	/* frag size in bytes */
	} d_partitions[NSECTIONS];
};

#  if defined(__STDC__) || defined(__cplusplus)
     extern struct disktab *getdiskbyname(char *);
#  else /* __STDC__ || __cplusplus */
     extern struct disktab *getdiskbyname();
#  endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* HFS */

#endif /* _DISKTAB_INCLUDED */
