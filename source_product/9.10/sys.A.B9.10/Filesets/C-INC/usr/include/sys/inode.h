/* @(#) $Revision: 1.41.83.5 $ */
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/inode.h,v $
 * $Revision: 1.41.83.5 $       $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/05/19 13:48:01 $
 */
#ifndef _SYS_INODE_INCLUDED /* allows multiple inclusion */
#define _SYS_INODE_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

/*
 * The I node is the focus of all file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An inode is 'named' by its dev/inumber pair. (iget/iget.c)
 * Data in icommon is read in from permanent inode on volume.
 */

#ifdef _KERNEL_BUILD
#include "../h/sem_beta.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/sem_beta.h>
#endif /* _KERNEL_BUILD */

#ifndef SITEARRAYSIZE
#ifdef _KERNEL_BUILD
#include "../dux/sitemap.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/sitemap.h>
#endif /* _KERNEL_BUILD */
#endif /* SITEARRAYSIZE */

#ifndef	_KERNEL_BUILD
#include <sys/vnode.h>
#endif /* ! _KERNEL_BUILD */

#ifdef _KERNEL_BUILD
#include "../h/acl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/acl.h>
#endif /* _KERNEL_BUILD */

#define	NDADDR	12		/* direct addresses in inode */
#define	NIADDR	3		/* indirect addresses in inode */
				/* fifo's depends on this value */
				/* if this value changes, look */
				/* at icommon.ic_un2.ic_reg.ic_un */

/*
 * Fast symlinks --
 *    symbolic links with paths short than MAX_FASTLINK_SIZE
 *    are stored in the inode where the direct and indirect
 *    block pointers are normally stored.  The flag IC_FASTLINK
 *    (in i_flags) indicates that the symbolic link is of the
 *    "fast" variety.
 *
 * This implementation cannot change, or the filesystem will
 * not be compatible with the OSF/1 "ufs" filesystem.
 */
#define MAX_FASTLINK_SIZE ((NDADDR + NIADDR) * sizeof (daddr_t))
#define IC_FASTLINK	0x00000001
#define IC_ASYNC	0x00000002	/*  write asynchronously  */

struct inode {
	struct	inode *i_chain[2];	/* must be first */
	struct	vnode i_vnode;	/* vnode associated with this inode */
	struct	vnode *i_devvp;	/* vnode for block i/o */
	u_int	i_flag;
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	int	i_diroff;	/* offset in dir, where we found last entry */
	struct	inode *i_contip; /* pointer to the continuation inode */
	struct	fs *i_fs;	/* file sys associated with this inode */
	struct	duxfs *i_dfs;
	struct	dquot *i_dquot;	/* quota structure controlling this file */

/*  Put the i_rdev here so the remote device stuff can change it
 *  and still have the real device number around
 */
	dev_t   i_rdev;		/* if special, the device number */

	union {
		daddr_t	if_lastr;	/* last read (read-ahead) */
		struct	socket *is_socket;
	} i_un;
	struct	{
		struct inode  *if_freef;	/* free list forward */
		struct inode **if_freeb;	/* free list back */
	} i_fr;
	struct i_select {
		struct proc *i_selp;
		short i_selflag;
	} i_fselr, i_fselw;
	struct locklist *i_locklist;	/* locked region list */
	struct sitemap i_opensites;	/* map of sites with file open */
	struct sitemap i_writesites;	/* map of sites writing to file */
	site_t i_ilocksite;		/* site holding ilock */
	short	i_pid;		/* pid of last process to lock this inode */
	union
	{
	struct sitemap is_execsites;	/* map of sites executing the file */
	struct sitemap is_fifordsites;	/* map of sites reading fifo */
	} i_siteu;
#define i_execsites i_siteu.is_execsites
#define i_fifordsites i_siteu.is_fifordsites
	struct dcount i_execdcount;	/* # of local process exec the file*/
	struct dcount i_refcount;	/* real and virtual reference counts*/
	struct sitemap i_refsites;	/* all other references */
	struct mount *i_mount;		/* mount table entry
					 * note this can be calculated as:
					 * (struct mount *)
					 *	(ITOV(ip)->v_vfsp->v_data)
					 * but since this is a relatively
					 * frequent operation in DUX, we
					 * save it here to make it more
					 * efficient.
					 */
	union
	{
	    struct icommon
	    {
		u_short	ic_mode;	/*  0: mode and type of file */
		short	ic_nlink;	/*  2: number of links to file */
		ushort	ic_uid;		/*  4: owner's user id */
		ushort  ic_gid;		/*  6: owner's group id */
		quad	ic_size;	/*  8: number of bytes in file */
#ifdef _KERNEL
		struct timeval ic_atime;/* 16: time last accessed */
		struct timeval ic_mtime;/* 24: time last modified */
		struct timeval ic_ctime;/* 32: last time inode changed */
#else
		time_t	ic_atime;	/* 16: time last accessed */
		long	ic_atspare;
		time_t	ic_mtime;	/* 24: time last modified */
		long	ic_mtspare;
		time_t	ic_ctime;	/* 32: last time inode changed */
		long	ic_ctspare;
#endif /* _KERNEL */
		union {
		    struct {
			daddr_t	ic_db[NDADDR];	/* 40: disk block addresses */
			union {
			    daddr_t ic_ib[NIADDR]; /* 88: indirect blocks */
			    struct ic_fifo
			    {
				short if_frptr;
				short if_fwptr;
				short if_frcnt;
				short if_fwcnt;
				short if_fflag;
				short if_fifosize;
			    } ic_fifo;
			} ic_un;
		    } ic_reg;
		    char ic_symlink[MAX_FASTLINK_SIZE];	/* 40: short symlink */
		} ic_un2;

		long	ic_flags;	/* 100: status */
		long	ic_blocks;	/* 104: blocks actually held */
		long	ic_gen;		/* 108: generation number */
		long	ic_fversion;	/* 112: file version number */
		long	ic_spare[2];	/* 116: reserved, currently unused */
		ino_t	ic_contin;	/* 124: continuation inode number */
	    } i_ic;
	    struct icont
	    {
		ushort	icc_mode;
		short	icc_nlink;	/*  2: number of links to file */
					/* 4: The optional entries of the
					 * access control list
					 */
#ifdef _KERNEL
		struct	acl_tuple icc_acl[NOPTTUPLES];
#else /* not _KERNEL */
		struct	acl_entry_internal icc_acl[NOPTENTRIES];
#endif /* else not _KERNEL */
		char	icc_spare[46];	/* 82: currently unused */
	    } i_icc;
	} i_icun;
#ifdef HPNSE
	struct stdata *i_sptr;	/* HP-UX NSE, associated stream */
#endif
	unsigned char i_ord_flags;	/* copied to buf for ordered writes */
};

#define L_REMOTE 0x1	/*  The process holding the lock is remote	*/

/* NOTE: Watch out for IWANT = 0x10, which is also used as a lock flag */
#define NFS_WANTS_LOCK 0x2	/* NFS lock manager is waiting for lock */

struct  locklist
{
     /* NOTE link must be first in struct */
     struct  locklist *ll_link;      /* link to next lock region */
     short   ll_count;		     /* reference count */
     short   ll_flags;               /* current flags: L_REMOTE, IWANT, ILBUSY */
     union
      { struct  proc *llu_proc;		/* process which owns region */
	struct
	  { site_t llur_psite;		/*  Site where process lives	*/
	    short  llur_pid;		/*  PID of process		*/
	  } llu_remote;
      } ll_u;

#define ll_proc  ll_u.llu_proc
#define ll_psite ll_u.llu_remote.llur_psite
#define ll_pid	 ll_u.llu_remote.llur_pid
     off_t   ll_start;               /* starting offset */
     off_t   ll_end;                 /* ending offset, zero is eof */
     short   ll_type;		     /* type of lock (for fnctl) */
     struct inode *ll_ip;	     /* Inode owning this locklist */
};
enum lockf_type {L_LOCKF, L_READ, L_WRITE, L_COPEN, L_FCNTL};

#define	i_mode		i_icun.i_ic.ic_mode
#define	i_nlink		i_icun.i_ic.ic_nlink
#define	i_uid		i_icun.i_ic.ic_uid
#define	i_gid		i_icun.i_ic.ic_gid
#define	i_size		i_icun.i_ic.ic_size.val[1]
#define	i_db		i_icun.i_ic.ic_un2.ic_reg.ic_db
#define	i_ib		i_icun.i_ic.ic_un2.ic_reg.ic_un.ic_ib
#define	i_atime		i_icun.i_ic.ic_atime
#define	i_mtime		i_icun.i_ic.ic_mtime
#define	i_ctime		i_icun.i_ic.ic_ctime
#define i_symlink	i_icun.i_ic.ic_un2.ic_symlink
#define i_flags		i_icun.i_ic.ic_flags
#define i_blocks	i_icun.i_ic.ic_blocks
/*  Define 1) new name for real device number 2) name for device site # */
#define i_device        i_icun.i_ic.ic_un2.ic_reg.ic_db[0]
#define i_rsite         i_icun.i_ic.ic_un2.ic_reg.ic_db[2]
#define i_gen		i_icun.i_ic.ic_gen
#define	i_lastr		i_un.if_lastr
#define	i_socket	i_un.is_socket
#define	i_forw		i_chain[0]
#define	i_back		i_chain[1]
#define	i_freef		i_fr.if_freef
#define	i_freeb		i_fr.if_freeb
#define i_frptr		i_icun.i_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_frptr
#define i_fwptr		i_icun.i_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_fwptr
#define i_frcnt		i_icun.i_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_frcnt
#define i_fwcnt		i_icun.i_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_fwcnt
#define i_fflag		i_icun.i_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_fflag
#define i_fifosize	i_icun.i_ic.ic_un2.ic_reg.ic_un.ic_fifo.if_fifosize
#define i_fifo		i_icun.i_ic.ic_un2.ic_reg.ic_un.ic_fifo
#define i_fversion	i_icun.i_ic.ic_fversion

#define	i_contin	i_icun.i_ic.ic_contin
#define i_acl		i_icun.i_icc.icc_acl


/*
 * Only include ino.h if we are defining _KERNEL.  No need otherwise.
 */
#ifdef _KERNEL
#ifdef _KERNEL_BUILD
#include "../ufs/ino.h"
#else /* ! _KERNEL_BUILD */
#include <sys/ino.h>
#endif /* _KERNEL_BUILD */
#endif /* _KERNEL */

#ifdef _KERNEL
#ifdef	__hp9000s800
extern	struct inode *inode;		/* the inode table itself */
extern	struct inode *inodeNINODE;	/* the end of the inode table */
extern	int	ninode;			/* number of slots in the table */

extern struct vnodeops ufs_vnodeops;	/* vnode operations for ufs */
extern struct vnodeops dux_vnodeops;	/* vnode operations for dux */

extern	struct	vnode *rootdir;		/* pointer to inode of root directory */
extern struct locklist locklist[]; /* The lock table itself */
#endif /* __hp9000s800 */

#ifdef __hp9000s300
struct inode *inode;		/* the inode table itself */
struct inode *inodeNINODE;	/* the end of the inode table */
int	ninode;			/* number of slots in the table */
extern struct vnodeops ufs_vnodeops;	/* vnode operations for ufs */
extern struct vnodeops dux_vnodeops;	/* vnode operations for dux */

struct	vnode *rootdir;			/* pointer to inode of root directory */
struct locklist locklist[]; /* The lock table itself */
#endif /* __hp9000s300 */

struct	inode *ialloc();
struct	inode *iget();
struct	inode *ifind();
struct	inode *owner();
struct	inode *maknode();
struct	inode *namei();

ino_t	dirpref();
#endif /* _KERNEL */

/* flags */
#define	ILOCKED		0x1		/* inode is locked */
#define	IUPD		0x2		/* file has been modified */
#define	IACC		0x4		/* inode access time to be updated */
#ifdef	notdef
#define	IMOUNT		0x8		/* inode is mounted on */
#endif
#define	IWANT		0x10		/* some process waiting on lock */
#define	ITEXT		0x20		/* inode is pure text prototype */
#define	ICHG		0x40		/* inode has been changed */
#ifdef	notdef
#define	ISHLOCK		0x80		/* file has shared lock */
#define	IEXLOCK		0x100		/* file has exclusive lock */
#endif
#define	ILWAIT		0x200		/* someone waiting on file lock */
#define IREF		0x400		/* inode is being referenced */
					/* change is use DUX !!! */
#define	ILBUSY		0x800		/* lock is not available */
#define IRENAME		0x1000		/* this inode is the source of a
					   rename operation */
#define IACLEXISTS	0x2000		/* An acl exists for this inode */


#define ISYNCLOCKED	0x10000		/* inode locked for synchronization */
#define ISYNC		0x20000		/* synchronous I/O required */
#define IDUXMNT		0x40000		/* inode mounted remotely */
#define ISYNCWANT	0x80000		/* a process waiting on ISYNCLOCKED */
#define IDUXMRT		0x100000	/* root inode of remotely mounted dev */
#define IBUFVALID	0x200000	/* incore buffers presumed valid */
#define IPAGEVALID	0x400000	/* incore exec pages presumed valid */
#define	IOPEN		0x800000	/* inode is currently being opened */

#define IFRAG		0x01000000	/* fragment was allocated, must refit */

#define IHARD           0x2000000       /* hardened inode */
#define INOFLUSH        0x4000000       /* for iflush */

#if defined(__hp9000s800) && !defined(_WSIO)
#define IF_MI_DEV	0x08000000	/* dev_t has mgr_index already */
#else /* __hp9000s800 */
#define IF_MI_DEV	0x00000000	/* s200 doesn't have mgr_index */
#endif /* __hp9000s800 */
#define IFRAGSYNC	0x10000000	/* need synch. frag_fit() */

/* modes */
#define	IFMT		0170000		/* type of file */
#define	IFIFO		0010000		/* fifo */
#define	IFCHR		0020000		/* character special */
#define	IFDIR		0040000		/* directory */
#define	IFBLK		0060000		/* block special */
#define IFCONT		0070000		/* continuation inode */
#define	IFREG		0100000		/* regular */
#define	IFNWK		0110000		/* network special */
#define	IFLNK		0120000		/* symbolic link */
#define	IFSOCK		0140000		/* socket */

#define	ISUID		04000		/* set user id on execution */
#define	ISGID		02000		/* set group id on execution */
#define IENFMT		02000		/* enforced file locking */
#define	ISVTX		01000		/* save swapped text even after use */
#define	IREAD		0400		/* read, write, execute permissions */
#define	IWRITE		0200
#define	IEXEC		0100

#define IFIR 		01		/* fifo read waiting for write flag */
#define	IFIW		02		/* fifo write waiting for read flag */
#define	PIPSIZ		8192		/* fifo buffer size */
#define	FSEL_COLL	01		/* select collision flag */

/* for ILOCK and related macros - PA */

#define	DUX_ILOCK(ip)	(ip)->i_ilocksite = u.u_site

#define	NFS_ILOCK(ip)	(ip)->i_pid = u.u_procp->p_pid

#ifdef QFS
#define	QFS_ILOCK(ip)	record_lock((int) ip)
#define	QFS_IUNLOCK(ip)	remove_lock((int) ip)
#else 	/* not QFS */
#define	QFS_ILOCK(ip)
#define	QFS_IUNLOCK(ip)
#endif 	/* not QFS */

#define	ILOCK(ip) { \
	QFS_ILOCK(ip);	\
	while ((ip)->i_flag & ILOCKED) { \
		(ip)->i_flag |= IWANT; \
		sleep((caddr_t)(ip), PINOD); \
	} \
	(ip)->i_flag |= ILOCKED; \
	DUX_ILOCK(ip);	\
	NFS_ILOCK(ip);	\
}

#define	IUNLOCK(ip) { \
	(ip)->i_flag &= ~ILOCKED; \
	QFS_IUNLOCK(ip); \
	if ((ip)->i_flag&IWANT) { \
		(ip)->i_flag &= ~IWANT; \
		wakeup((caddr_t)(ip)); \
	} \
}

#ifdef _KERNEL
/*
 * Convert between inode pointers and vnode pointers
 */
#define VTOI(VP)	((struct inode *)(VP)->v_data)
#define ITOV(IP)	((struct vnode *)&(IP)->i_vnode)

/*
 * Convert between vnode types and inode formats
 */
extern enum vtype	iftovt_tab[];
extern int		vttoif_tab[];
#define IFTOVT(M)	((((M)&IFMT) == IFNWK)?VFNWK:((((M)&IFMT) == IFIFO)?VFIFO:(iftovt_tab[(((M) & IFMT)) >> 13])))
#define VTTOIF(T)	(vttoif_tab[(int)(T)])

#define MAKEIMODE(T, M)	(VTTOIF(T) | (M))

#define ESAME (-1)		/* trying to rename linked files (special) */
#ifdef __hp9000s300
#define EREMOVE (-2)		/* "source" file of link removed in the
				    middle of operation (happens only
				    originate from client)*/
#endif /* __hp9000s300 */
#ifdef __hp9000s800
#define EREMOVE (-2)		/* "source" file of link removed in the
				    middle of operation (happens only
				    originate from client)*/
#endif /* __hp9000s800 */
#define ERENAME (-3)		/* the inode being rename'd is in the path
				   of another rename operation*/
#define EPATHCONF_NONAME (-4)	/* The posix standard says that if a user
				   requests an unknown name, it should not
				   change errorno but should return an error.
				   This indicates that is the case. */

/*
 * Check that file is owned by current user or user is su.
 */
/* We can't do a straight comparision of (CR)->cr_uid against (IP)->i_uid.
 * We also need to check the case where we are NFS, and network root (-2)
 * and the inode is owned by "nobody" because i_uid is an ushort and -2 is
 * stored as 65534.
 */
/* name conflict with DIL */
#define OWNER_CR(CR, IP) \
    (((CR)->cr_uid == (IP)->i_uid)? 0: \
	((((CR)->cr_uid == -2) && ((IP)->i_uid == (ushort)-2))? 0: \
	    (suser()? 0: u.u_error)))

/*
 * enums
 */
enum de_op	{ DE_CREATE, DE_LINK, DE_RENAME };	/* direnter ops */

#endif /* _KERNEL */
/*
 * This overlays the fid structure (see vfs.h).  Used mainly in support
 * of NFS 3.2 file handles, the fid structure should contain the minimum
 * information necessary to uniquely identify a file, GIVEN a pointer to
 * the file system.
 */
struct ufid {
	u_short	ufid_len;
	u_short pad;
	ino_t	ufid_ino;
	long	ufid_gen;
};

#endif /* ! _SYS_INODE_INCLUDED */
