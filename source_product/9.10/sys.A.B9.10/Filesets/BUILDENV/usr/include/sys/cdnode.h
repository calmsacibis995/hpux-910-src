/*
 * @(#)cdnode.h: $Revision: 1.6.83.3 $ $Date: 93/09/17 16:37:24 $
 * $Locker:  $
 */

/* @(#) $Revision: 1.6.83.3 $ */    
#ifndef _SYS_CDNODE_INCLUDED /* allows multiple inclusion */
#define _SYS_CDNODE_INCLUDED

/*
 * The I node is the focus of all file activity in UNIX.
 * There is a unique cdnode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An cdnode is 'named' by its dev/inumber pair. (iget/iget.c)
 * Data in icommon is read in from permanent cdnode on volume.
 */

#ifdef _KERNEL_BUILD
#include "../dux/sitemap.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/sitemap.h>
#endif /* _KERNEL_BUILD */

#ifndef	_KERNEL_BUILD
#include <sys/vnode.h>
#endif /* ! _KERNEL_BUILD */

struct	two_time {
	u_int	post_time;
	u_int	pre_time;
};

struct cdnode {
	struct	cdnode *cd_chain[2];	/* must be first */
	struct	vnode cd_vnode;	/* vnode associated with this cdnode */
	struct	vnode *cd_devvp;	/* vnode for block i/o */
	u_int	cd_format;	/*HSGFS/ISO9660FS*/
	u_int	cd_flag;
	dev_t	cd_dev;		/* device where cdnode resides */
	cdno_t	cd_pnum;		/* a number unique identify parent dir*/ 
	cdno_t	cd_num;		/* a number unique identify a file in a 
				   file system. */
	cdno_t	cd_nextnum;	/* cd_number of next file section*/
	int	cd_diroff;	/* offset in dir, where we found last entry */
	struct	cdfs *cd_fs;	/* file sys associated with this cdnode */
	

/*  Put the cd_rdev here so the remote device stuff can change it
 *  and still have the real device number around
 */
	dev_t   cd_rdev;		/* if special, the device number */
	struct duxfs	*cd_dfs;

	daddr_t	cd_nextr;	/* next read (read-ahead) */
	struct {
		struct cdnode *cdf_freef;
		struct cdnode **cdf_freeb;
	} cd_fr;
	struct sitemap cd_opensites;	/*map of sites with file open*/
	struct sitemap cd_execsites;	/*map of sites executing the file*/
	site_t	cd_cdlocksite;
	struct dcount cd_execdcount;	/*# of local process exec the file*/
	struct dcount cd_refcount;	/*real and virtual reference counts*/
	struct sitemap cd_refsites;	/*all other references*/
	struct mount *cd_mount;		/*mount table entry
					 *note this can be calculated as:
					 * (struct mount *)
					 *	(CDTOV(cdp)->v_vfsp->v_data)
					 *but since this is a relatively
					 *frequent operation in DUX, we
					 *save it here to make it more
					 *efficient.
					 */
	int	cd_spare[8];		/*reserve for future use*/
	struct 	cdcommon		/*info from file descriptor/xar on disc*/
	{
		u_int	cdc_size;	/*  : number of bytes in file */
		u_int	cdc_seq;		/*  : volume that the file is on*/
		u_int	cdc_loc;		/*  : location of file(in logic block)*/
		u_char	cdc_fusize;	/*  : file unit size in logic block */
		u_char	cdc_xarlen;	/*  : length of XAR in logic block*/
		u_char	cdc_fugsize;	/*  : file unit gap size in logic 
					      block */
		u_char	cdc_ftype;
		u_char	cdc_fflag;
		ushort	cdc_uid;		/*  : owner's user id */
		ushort  cdc_gid;		/*  : owner's group id */
		u_short	cdc_mode;	/*  : mode and type of file */
		long	cdc_gen;		/* : generation number */
		struct two_time cdc_record_t; /* recording time */
		char	cdc_create_year[YEAR_DIGIT];
		char	cdc_create_month[MONTH_DIGIT];
		char	cdc_create_day[DAY_DIGIT];
		char	cdc_create_hour[HOUR_DIGIT];
		char	cdc_create_minute[MINUTE_DIGIT];
		char	cdc_create_second[SECOND_DIGIT];
		char	cdc_create_zone[ZONE_DIGIT];
		char	cdc_mod_year[YEAR_DIGIT];
		char	cdc_mod_month[MONTH_DIGIT];
		char	cdc_mod_day[DAY_DIGIT];
		char	cdc_mod_hour[HOUR_DIGIT];
		char	cdc_mod_minute[MINUTE_DIGIT];
		char	cdc_mod_second[SECOND_DIGIT];
		char	cdc_mod_zone[ZONE_DIGIT];
		char	cdc_exp_year[YEAR_DIGIT];
		char	cdc_exp_month[MONTH_DIGIT];
		char	cdc_exp_day[DAY_DIGIT];
		char	cdc_exp_hour[HOUR_DIGIT];
		char	cdc_exp_minute[MINUTE_DIGIT];
		char	cdc_exp_second[SECOND_DIGIT];
		char	cdc_exp_zone[ZONE_DIGIT];
		char	cdc_eff_year[YEAR_DIGIT];
		char	cdc_eff_month[MONTH_DIGIT];
		char	cdc_eff_day[DAY_DIGIT];
		char	cdc_eff_hour[HOUR_DIGIT];
		char	cdc_eff_minute[MINUTE_DIGIT];
		char	cdc_eff_second[SECOND_DIGIT];
		char	cdc_eff_zone[ZONE_DIGIT];
		long	cdc_fversion;	/* : file version number */
		long	cdc_spare[3];	/* : reserved, currently unused */
	} cd_cdc;
};


#define	cd_mode		cd_cdc.cdc_mode
#define	cd_xarlen	cd_cdc.cdc_xarlen
#define	cd_ftype		cd_cdc.cdc_ftype
#define	cd_fflag		cd_cdc.cdc_fflag
#define	cd_uid		cd_cdc.cdc_uid
#define	cd_gid		cd_cdc.cdc_gid
#define	cd_size		cd_cdc.cdc_size
#define	cd_seq		cd_cdc.cdc_seq
#define	cd_loc		cd_cdc.cdc_loc
#define	cd_fusize	cd_cdc.cdc_fusize
#define	cd_fugsize	cd_cdc.cdc_fugsize
#define	cd_record_t	cd_cdc.cdc_record_t
#define cd_gen		cd_cdc.cdc_gen
#define	cd_forw		cd_chain[0]
#define	cd_back		cd_chain[1]
#define cd_fversion	cd_cdc.cdc_fversion
#define cd_freef		cd_fr.cdf_freef
#define cd_freeb		cd_fr.cdf_freeb
#define	cd_create_year	cd_cdc.cdc_create_year
#define	cd_create_month	cd_cdc.cdc_create_month
#define	cd_create_day	cd_cdc.cdc_create_day
#define	cd_create_hour	cd_cdc.cdc_create_hour
#define	cd_create_minute	cd_cdc.cdc_create_minute
#define	cd_create_second	cd_cdc.cdc_create_second
#define	cd_create_zone	cd_cdc.cdc_create_zone
#define	cd_mod_year	cd_cdc.cdc_mod_year
#define	cd_mod_month	cd_cdc.cdc_mod_month
#define	cd_mod_day	cd_cdc.cdc_mod_day
#define	cd_mod_hour	cd_cdc.cdc_mod_hour
#define	cd_mod_minute	cd_cdc.cdc_mod_minute
#define	cd_mod_second	cd_cdc.cdc_mod_second
#define	cd_mod_zone	cd_cdc.cdc_mod_zone
#define	cd_exp_year	cd_cdc.cdc_exp_year
#define	cd_exp_month	cd_cdc.cdc_exp_month
#define	cd_exp_day	cd_cdc.cdc_exp_day
#define	cd_exp_hour	cd_cdc.cdc_exp_hour
#define	cd_exp_minute	cd_cdc.cdc_exp_minute
#define	cd_exp_second	cd_cdc.cdc_exp_second
#define	cd_exp_zone	cd_cdc.cdc_exp_zone
#define	cd_eff_year	cd_cdc.cdc_eff_year
#define	cd_eff_month	cd_cdc.cdc_eff_month
#define	cd_eff_day	cd_cdc.cdc_eff_day
#define	cd_eff_hour	cd_cdc.cdc_eff_hour
#define	cd_eff_minute	cd_cdc.cdc_eff_minute
#define	cd_eff_second	cd_cdc.cdc_eff_second
#define	cd_eff_zone	cd_cdc.cdc_eff_zone

#ifdef _KERNEL

struct cdnode *cdnode;		/* the cdnode table itself */
struct cdnode *cdnodeNCDNODE;	/* the end of the cdnode table */
extern int	ncdnode;	/* number of slots in the table */
extern struct vnodeops cdfs_vnodeops;	/* vnode operations for cdfs */
extern struct vnodeops dux_vnodeops;	/* vnode operations for dux */

struct	vnode *rootdir;			/* pointer to cdnode of root directory */

struct	cdnode *cdalloc();
struct	cdnode *cdget();
struct	cdnode *makcdnode();
struct	cdnode *cdfind();

#endif

/* flags */
#define	CDLOCKED		0x1		/* cdnode is locked */
#define	CDMOUNT		0x8		/* cdnode is mounted on */
#define	CDWANT		0x10		/* some process waiting on lock */
#define CDREF		0x400		/* cdnode is being referenced */
#define CDDUXMNT	0x40000		/* cdnode mounted remotely */
#define CDDUXMRT	0x100000	/* root cdnode of remotely mounted dev */
#define CDBUFVALID	0x200000	/* incore buffers presumed valid */
#define CDPAGEVALID	0x400000	/* incore exec pages presumed valid */
#define	CDOPEN		0x800000	/* cdnode is currently being opened */

/* file type */
#define	CDFDIR		0000002		/* directory */
#define	CDFREG		0000001		/* regular */

/* modes */
#define	CDREAD		0400		/* read, write, execute permissions */
#define	CDWRITE		0200		/* read, write, execute permissions */
#define	CDEXEC		0100
/*#define	CDLBUSY		0x800		/* lock is not available */


#ifdef _KERNEL
/*
 * Convert between cdnode pointers and vnode pointers
 */
#define VTOCD(VP)	((struct cdnode *)(VP)->v_data)
#define CDTOV(CDP)	((struct vnode *)&(((struct cdnode *)CDP)->cd_vnode))

/*
 * Convert between vnode types and cdnode formats
 */
extern enum vtype	cdftovt_tab[];
extern int		vttocdf_tab[];
#define CDFTOVT(ftype)	((ftype & CDFDIR) ? VDIR : VREG)
#define VTTOCDF(T)	(vttocdf_tab[(int)(T)])

#define MAKECDMODE(T, M)	(VTTOCDF(T) | (M))

/*
 * Lock and unlock cdnodes.
 */
#define	CDLOCK(cdp) { \
	while ((cdp)->cd_flag & CDLOCKED) { \
		(cdp)->cd_flag |= CDWANT; \
		sleep((caddr_t)(cdp), PINOD); \
	} \
	(cdp)->cd_flag |= CDLOCKED; \
	(cdp)->cd_cdlocksite = u.u_site; \
}

#define	CDUNLOCK(cdp) { \
	(cdp)->cd_flag &= ~CDLOCKED; \
	if ((cdp)->cd_flag&CDWANT) { \
		(cdp)->cd_flag &= ~CDWANT; \
		wakeup((caddr_t)(cdp)); \
	} \
}


/*
 * Check that file is owned by current user or user is su.
 */
#define CDOWNER_CR(CR, CDP)	((((CDP)->cd_uid == 0) || ((CR)->cr_uid == (CDP)->cd_uid))? 0: (suser()? 0: u.u_error))
/*
 * enums
 */

#endif
/*
 * This overlays the fid structure (see vfs.h).  Used mainly in support
 * of NFS 3.2 file handles, the fid structure should contain the minimum
 * information necessary to uniquely identify a file, GIVEN a pointer to
 * the file system.
 */
struct cdfid {
	u_short	cdfid_len;
	cdno_t	cdfid_cdno;
	long	cdfid_gen;
};

#endif /* _SYS_CDNODE_INCLUDED */
