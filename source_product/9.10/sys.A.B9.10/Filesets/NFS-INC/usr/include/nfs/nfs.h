/*
 * @(#)nfs.h: $Revision: 1.11.83.4 $ $Date: 93/09/17 19:07:39 $
 * $Locker:  $
 */

#ifndef _NFS_NFS_INCLUDED
#define _NFS_NFS_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#include "../h/errno.h"
#include "../h/time.h"
#include "../rpc/types.h"
#include "../h/uio.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/errno.h>
#include <sys/time.h>
#include <rpc/types.h>
#include <sys/uio.h>
#endif /* _KERNEL_BUILD */

#ifndef _NFS_AUTOMOUNT

/* Maximum size of data portion of a remote request */
#define	NFS_MAXDATA	8192
#define	NFS_MAXNAMLEN	255
#define	NFS_MAXPATHLEN	1024
#define NFS_MAXIOVEC    16

/*
 * Rpc retransmission parameters
 */
#define       NFS_TIMEO       7       /* initial timeout in tenths of a sec. */
#define       NFS_RETRIES     4       /* times to retry request */

/*
 * maximum transfer size for different interfaces
 */
#define ECTSIZE 4096
#define IETSIZE 8192

/*
 * Error status
 * Should include all possible net errors.
 * For now we just cast errno into an enum nfsstat.
 * NOTE: These constants are defined using values in errno.h.  Therefore,
 *    the actual values used may differ between implementations, requiring
 *    translations to be done to values sent over the net.
 */
enum nfsstat {
	NFS_OK = 0,			/* no error */
	NFSERR_PERM=EPERM,		/* Not owner */
	NFSERR_NOENT=ENOENT,		/* No such file or directory */
	NFSERR_IO=EIO,			/* I/O error */
	NFSERR_NXIO=ENXIO,		/* No such device or address */
	NFSERR_ACCES=EACCES,		/* Permission denied */
	NFSERR_EXIST=EEXIST,		/* File exists */
	NFSERR_NODEV=ENODEV,		/* No such device */
	NFSERR_NOTDIR=ENOTDIR,		/* Not a directory*/
	NFSERR_ISDIR=EISDIR,		/* Is a directory */
	NFSERR_FBIG=EFBIG,		/* File too large */
	NFSERR_NOSPC=ENOSPC,		/* No space left on device */
	NFSERR_ROFS=EROFS,		/* Read-only file system */
	NFSERR_NAMETOOLONG=ENAMETOOLONG,/* File name too long */
	NFSERR_NOTEMPTY=ENOTEMPTY,	/* Directory not empty */
	NFSERR_DQUOT=EDQUOT,		/* Disc quota exceeded */
	NFSERR_STALE=ESTALE,		/* Stale NFS file handle */
	NFSERR_WFLUSH			/* write cache flushed */
};
#define	puterrno(error)		((enum nfsstat)error)
#define	geterrno(status)	((int)status)

/*
 * File types
 */
enum nfsftype {
	NFNON,
	NFREG,		/* regular file */
	NFDIR,		/* directory */
	NFBLK,		/* block special */
	NFCHR,		/* character special */
	NFLNK		/* symbolic link */
};

/*
 * Special workaround for fifos (named pipes)  [to adhere to NFS Protocol Spec]
 *
 * VFIFO is not in the protocol spec (VNON will be replaced by VFIFO)
 * so the over-the-wire representation is VCHR with a '-1' device number.
 *
 * NOTE: This may become unnecessary with a Protocol Revision,
 *       but it may be necessary to support it (backwards compatibility).
 */
#define NFS_FIFO_TYPE	NFCHR
#define NFS_FIFO_MODE	S_IFCHR
#define NFS_FIFO_DEV	(~0)

/* identify fifo in nfs attributes */
#define NA_ISFIFO(NA)	(((NA)->na_type == NFS_FIFO_TYPE) && \
			    ((NA)->na_rdev == NFS_FIFO_DEV))

/* set fifo in nfs attributes */
#define NA_SETFIFO(NA)	{ \
			(NA)->na_type = NFS_FIFO_TYPE; \
			(NA)->na_rdev = NFS_FIFO_DEV; \
			(NA)->na_mode = ((NA)->na_mode&~S_IFMT)|NFS_FIFO_MODE; \
			}


/*
 * Size of an fhandle in bytes
 */
#define	NFS_FHSIZE	32

/*
 * File access handle (NFS 3.2 version )
 * This structure is the Sun server representation of a file.
 * It is handed out by a server for the client to use in further
 * file transactions.
 */

#ifdef NFSSERVER	/* NFSSERVER should be defined in nfs_server.c */
/*
 * This struct is only used to find the size of the data field in the 
 * fhandle structure below.
 *
 * NOTE:  Need to be really careful with alignment
 * here to avoid having some structures the wrong size.  The "data" will
 * end up being, for example, a ufs inode and generation number.  This
 * is accomplished by calling VFS_VGET() with the address of fh_len, which
 * must correspond to the begining of the ufid structure as seen by
 * the ufs or other layers.
 */
static
struct fhsize {
	fsid_t	f1;
	u_short	f2;
	char	f3[4];
	u_short	f4;
	char	f5[4];
};
#define NFS_FHMAXDATA	((NFS_FHSIZE - sizeof (struct fhsize) + 8) / 2)

struct svcfh {
	fsid_t	fh_fsid;		/* filesystem id */
	u_short	fh_len;			/* file number length */
	char	fh_data[NFS_FHMAXDATA];	/* and data */
	u_short	fh_xlen;		/* export file number length */
	char	fh_xdata[NFS_FHMAXDATA];/* and data */
};

typedef struct svcfh fhandle_t;
#else
/*
 * This is the client view of an fhandle
 */
typedef struct {
	char	fh_data[NFS_FHSIZE];	/* opaque data */
} fhandle_t;
#endif



/*
 * This structure is used to do the translations between local errno values,
 * and values that are passed out over the net.  See defines of nfsstat above.
 */

struct gen_result {
	enum nfsstat   gen_status;
};

/*
 * Arguments to remote write and writecache
 */
struct nfswriteargs {
	fhandle_t	wa_fhandle;	/* handle for file */
	u_long		wa_begoff;	/* beginning byte offset in file */
	u_long		wa_offset;      /* current byte offset in file */
	u_long		wa_totcount;    /* total write count (to this offset)*/
	u_long		wa_count;	/* size of this write */
	char		*wa_data;	/* data to write (up to NFS_MAXDATA) */
	int             wa_iovcnt;      /* NIKE - amount of wa_iov used */
	struct iovec    wa_iov[NFS_MAXIOVEC]; 
					/* NIKE - frags of data on serv*/

};


/*
 * File attributes
 */
struct nfsfattr {
	enum nfsftype	na_type;	/* file type */
	u_long		na_mode;	/* protection mode bits */
	u_long		na_nlink;	/* # hard links */
	u_long		na_uid;		/* owner user id */
	u_long		na_gid;		/* owner group id */
	u_long		na_size;	/* file size in bytes */
	u_long		na_blocksize;	/* prefered block size */
	u_long		na_rdev;	/* special device # */
	u_long		na_blocks;	/* Kb of disk used by file */
	u_long		na_fsid;	/* device # */
	u_long		na_nodeid;	/* inode # */
	struct timeval	na_atime;	/* time of last access */
	struct timeval	na_mtime;	/* time of last modification */
	struct timeval	na_ctime;	/* time of last change */
};

#define n2v_type(x)	(NA_ISFIFO(x) ? VFIFO : (enum vtype)((x)->na_type))
#define n2v_rdev(x)	(NA_ISFIFO(x) ? 0 : (x)->na_rdev)

/*
 * Arguments to remote read
 */
struct nfsreadargs {
	fhandle_t	ra_fhandle;	/* handle for file */
	u_long		ra_offset;	/* byte offset in file */
	u_long		ra_count;	/* immediate read count */
	u_long		ra_totcount;	/* total read count (from this offset)*/
};

/*
 * Status OK portion of remote read reply
 */
struct nfsrrok {
	struct nfsfattr	rrok_attr;	/* attributes, need for pagin*/
	u_long		rrok_count;	/* bytes of data */
	char		*rrok_data;	/* data (up to NFS_MAXDATA bytes) */
	struct buf	*rrok_bp;	/* buffer pointer for bread */
	struct vnode	*rrok_vp;	/* vnode assoc. with buffer */
};

/*
 * Reply from remote read
 */
struct nfsrdresult {
	enum nfsstat	rr_status;		/* status of read */
	union {
		struct nfsrrok	rr_ok_u;	/* attributes, need for pagin*/
	} rr_u;
};
#define	rr_ok		rr_u.rr_ok_u
#define	rr_attr		rr_u.rr_ok_u.rrok_attr
#define	rr_count	rr_u.rr_ok_u.rrok_count
#define	rr_data		rr_u.rr_ok_u.rrok_data
#define rr_bp		rr_u.rr_ok_u.rrok_bp
#define rr_vp		rr_u.rr_ok_u.rrok_vp


/*
 * File attributes which can be set
 */
struct nfssattr {
	u_long		sa_mode;	/* protection mode bits */
	u_long		sa_uid;		/* owner user id */
	u_long		sa_gid;		/* owner group id */
	u_long		sa_size;	/* file size in bytes */
	struct timeval	sa_atime;	/* time of last access */
	struct timeval	sa_mtime;	/* time of last modification */
};


/*
 * Reply status with file attributes
 */
struct nfsattrstat {
	enum nfsstat	ns_status;		/* reply status */
	union {
		struct nfsfattr ns_attr_u;	/* NFS_OK: file attributes */
	} ns_u;
};
#define	ns_attr	ns_u.ns_attr_u


/*
 * NFS_OK part of read sym link reply union
 */
struct nfssrok {
	u_long	srok_count;	/* size of string */
	char	*srok_data;	/* string (up to NFS_MAXPATHLEN bytes) */
};

/*
 * Result of reading symbolic link
 */
struct nfsrdlnres {
	enum nfsstat	rl_status;		/* status of symlink read */
	union {
		struct nfssrok	rl_srok_u;	/* name of linked to */
	} rl_u;
};
#define	rl_srok		rl_u.rl_srok_u
#define	rl_count	rl_u.rl_srok_u.srok_count
#define	rl_data		rl_u.rl_srok_u.srok_data


/*
 * Arguments to readdir
 */
struct nfsrddirargs {
	fhandle_t rda_fh;	/* directory handle */
	u_long rda_offset;	/* offset in directory (opaque) */
	u_long rda_count;	/* number of directory bytes to read */
};

/*
 * NFS_OK part of readdir result
 */
struct nfsrdok {
	u_long	rdok_offset;		/* next offset (opaque) */
	u_long	rdok_size;		/* size in bytes of entries */
	bool_t	rdok_eof;		/* true if last entry is in result*/
	struct direct *rdok_entries;	/* variable number of entries*/
	struct direct *rdok_mem;	/* addr of allocated storage for */
					/*    rdok_entries */
};

/*
 * Readdir result
 * NOTE: The rd_status field was moved to be the first field in the structure
 * to match all the other NFS reply structures.  This is convenient for
 * checking the reply status without knowing what type of structure it is.
 * This is legal since the structure is NOT what is actually sent over the
 * network to the client, only the interpretation of the structure.
 */
struct nfsrddirres {
	enum nfsstat	rd_status;
	u_long		rd_bufsize;	/* size of client request (not xdr'ed)*/
	union {
		struct nfsrdok rd_rdok_u;
	} rd_u;
};
#define	rd_rdok		rd_u.rd_rdok_u
#define	rd_offset	rd_u.rd_rdok_u.rdok_offset
#define	rd_size		rd_u.rd_rdok_u.rdok_size
#define	rd_eof		rd_u.rd_rdok_u.rdok_eof
#define	rd_entries	rd_u.rd_rdok_u.rdok_entries
#define rd_mem		rd_u.rd_rdok_u.rdok_mem


/*
 * Arguments for directory operations
 */
struct nfsdiropargs {
	fhandle_t	da_fhandle;	/* directory file handle */
	char		*da_name;	/* name (up to NFS_MAXNAMLEN bytes) */
};

/*
 * NFS_OK part of directory operation result
 */
struct  nfsdrok {
	fhandle_t	drok_fhandle;	/* result file handle */
	struct nfsfattr	drok_attr;	/* result file attributes */
};

/*
 * Results from directory operation 
 */
struct  nfsdiropres {
	enum nfsstat	dr_status;	/* result status */
	union {
		struct  nfsdrok	dr_drok_u;	/* NFS_OK result */
	} dr_u;
};
#define	dr_drok		dr_u.dr_drok_u
#define	dr_fhandle	dr_u.dr_drok_u.drok_fhandle
#define	dr_attr		dr_u.dr_drok_u.drok_attr

/*
 * arguments to setattr
 */
struct nfssaargs {
	fhandle_t	saa_fh;		/* fhandle of file to be set */
	struct nfssattr	saa_sa;		/* new attributes */
};

/*
 * arguments to create and mkdir
 */
struct nfscreatargs {
	struct nfsdiropargs	ca_da;	/* file name to create and parent dir */
	struct nfssattr		ca_sa;	/* initial attributes */
};

/*
 * arguments to link
 */
struct nfslinkargs {
	fhandle_t		la_from;	/* old file */
	struct nfsdiropargs	la_to;		/* new file and parent dir */
};

/*
 * arguments to rename
 */
struct nfsrnmargs {
	struct nfsdiropargs rna_from;	/* old file and parent dir */
	struct nfsdiropargs rna_to;	/* new file and parent dir */
};

/*
 * arguments to symlink
 */
struct nfsslargs {
	struct nfsdiropargs	sla_from;	/* old file and parent dir */
	char			*sla_tnm;	/* new name */
	struct nfssattr		sla_sa;		/* attributes */
};

/*
 * NFS_OK part of statfs operation
 */
struct nfsstatfsok {
	u_long fsok_tsize;	/* preferred transfer size in bytes */
	u_long fsok_bsize;	/* fundamental file system block size */
	u_long fsok_blocks;	/* total blocks in file system */
	u_long fsok_bfree;	/* free blocks in fs */
	u_long fsok_bavail;	/* free blocks avail to non-superuser */
};

/*
 * Results of statfs operation
 */
struct nfsstatfs {
	enum nfsstat	fs_status;	/* result status */
	union {
		struct	nfsstatfsok fs_fsok_u;	/* NFS_OK result */
	} fs_u;
};
#define	fs_fsok		fs_u.fs_fsok_u
#define	fs_tsize	fs_u.fs_fsok_u.fsok_tsize
/*
 * Note: fs_bsize was changed to fs_bsize_nfs to avoid name conflict with fs.h
 */
#define	fs_bsize_nfs	fs_u.fs_fsok_u.fsok_bsize
#define	fs_blocks	fs_u.fs_fsok_u.fsok_blocks
#define	fs_bfree	fs_u.fs_fsok_u.fsok_bfree
#define	fs_bavail	fs_u.fs_fsok_u.fsok_bavail

/*
 * XDR routines for handling structures defined above
 */
bool_t xdr_attrstat();
bool_t xdr_creatargs();
bool_t xdr_diropargs();
bool_t xdr_diropres();
bool_t xdr_drok();
bool_t xdr_fattr();
bool_t xdr_fhandle();
bool_t xdr_linkargs();
bool_t xdr_rddirargs();
bool_t xdr_putrddirres();
bool_t xdr_getrddirres();
bool_t xdr_rdlnres();
bool_t xdr_rdresult();
bool_t xdr_readargs();
bool_t xdr_rnmargs();
bool_t xdr_rrok();
bool_t xdr_saargs();
bool_t xdr_sattr();
bool_t xdr_slargs();
bool_t xdr_srok();
bool_t xdr_timeval();
bool_t xdr_writeargs();
bool_t xdr_statfs();

/*
 * Remote file service routines
 */
#define	RFS_NULL	0
#define	RFS_GETATTR	1
#define	RFS_SETATTR	2
#define	RFS_ROOT	3
#define	RFS_LOOKUP	4
#define	RFS_READLINK	5
#define	RFS_READ	6
#define	RFS_WRITECACHE	7
#define	RFS_WRITE	8
#define	RFS_CREATE	9
#define	RFS_REMOVE	10
#define	RFS_RENAME	11
#define	RFS_LINK	12
#define	RFS_SYMLINK	13
#define	RFS_MKDIR	14
#define	RFS_RMDIR	15
#define	RFS_READDIR	16
#define	RFS_STATFS	17
#define	RFS_NPROC	18

/*
 * remote file service numbers
 */
#define	NFS_PROGRAM	((u_long)100003)
#define	NFS_VERSION	((u_long)2)
#define	NFS_PORT	2049

#endif /* not _NFS_AUTOMOUNT */

struct nfs_args {
	struct sockaddr_in	*addr;		/* file server address */
	fhandle_t		*fh;		/* File handle to be mounted */
	int			flags;		/* flags */
	int			wsize;		/* write size in bytes */
	int			rsize;		/* read size in bytes */
	int			timeo;		/* initial timeout in .1 secs */
	int			retrans;	/* times to retry send */
	char			*hostname;	/* server's name */
	int			acregmin;	/* attr cache file min secs */
	int			acregmax;	/* attr cache file max secs */
	int			acdirmin;	/* attr cache dir min secs */
	int			acdirmax;	/* attr cache dir max secs */
	char			*fsname;	/* server's fs path name */
};

/*
 * NFS mount option flags
 */
#define	NFSMNT_SOFT	0x001	/* soft mount (hard is default) */
#define	NFSMNT_WSIZE	0x002	/* set write size */
#define	NFSMNT_RSIZE	0x004	/* set read size */
#define	NFSMNT_TIMEO	0x008	/* set initial timeout */
#define	NFSMNT_RETRANS	0x010	/* set number of request retrys */
#define	NFSMNT_HOSTNAME	0x020	/* set hostname for error printf */
#define	NFSMNT_INT	0x040	/* set option to have interruptable mounts */
#define NFSMNT_NODEVS   0x080   /* turn off device file access (default on) */
#define NFSMNT_FSNAME   0x100   /* provide name of server's fs to system */
#define NFSMNT_IGNORE   0x200   /* mark this file system as ignore in mnttab */
#define NFSMNT_NOAC     0x400   /* don't cache file attributes */
#define NFSMNT_NOCTO    0x800   /* don't get new attributes on open */
#define NFSMNT_DYNAMIC  0x1000  /* Use dynamic read and write sizes */
#define NFSMNT_ACREGMIN 0x02000 /* set min secs for file attr cache */
#define NFSMNT_ACREGMAX 0x04000 /* set max secs for file attr cache */
#define NFSMNT_ACDIRMIN 0x08000 /* set min secs for dir attr cache */
#define NFSMNT_ACDIRMAX 0x10000 /* set max secs for dir attr cache */


/*
* NFS export option strings 
*/
#define NFSEXP_ASYNC_STR  "async"   /* cause asynchronous writes to this file
                                      system */

/* 
* NFS export option flages
*/
#define NFSEXP_NO_OPTS    0x0000   /* used to initialize export option flags */
#define NFSEXP_ASYNC      0x0001   /* flag to indicate that asynchronous writes
                                   should be used for a given file system */


/* Function prototypes */

#ifndef _KERNEL
#ifdef __cplusplus
  extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
   extern int getdomainname(char *, int);
   extern int setdomainname(const char *, int);
   extern int nfssvc(int);
   extern void async_daemon(void);
   extern int getfh(int, const fhandle_t *);
   extern int exportfs(const char *, const struct export *);
#else /* not _PROTOTYPES */
   extern int getdomainname();
   extern int setdomainname();
   extern int nfssvc();
   extern void async_daemon();
   extern int getfh();
   extern int exportfs();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
  }
#endif /* __cplusplus */
#endif /* not _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _NFS_NFS_INCLUDED */
