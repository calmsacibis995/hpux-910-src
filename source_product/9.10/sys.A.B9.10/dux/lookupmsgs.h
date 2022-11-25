/*
 * @(#)lookupmsgs.h: $Revision: 1.5.83.3 $ $Date: 93/09/17 16:44:19 $
 * $Locker:  $
 */

/* HPUX_ID: %W%		%E% */
/*
 *This file contains the structures used by the individual system calls that
 *perform lookup operations.
 */
/*
 *WARNING:  All structures tagged with "DUX MESSAGE STRUCTURE" are passed
 *between machines.  They must obey the following rules:
 *	1)  All integers (and larger) must be 32 bit aligned
 *	2)  They must be consistant between all versions of DUX in the
 *	cluster.
 */

/* Pure lookup reply (note request is just lookup stuff)*/
#define	LOOKUP_REPLY_COMPVPP	1
#define	LOOKUP_REPLY_DIRVPP	2
struct pure_lookup_reply	/*DUX MESSAGE STRUCTURE*/
{
	int	lookup_reply_flag;
	struct remvno pkv;
	struct remvno pkvdvp;  /*reply dvp as well*/
};

#define pkcd pkv.rv_cdno
#define pkcddvp pkvdvp.rv_cdno
#define pki pkv.rv_ino
#define pkidvp pkvdvp.rv_ino

struct iref_update	/*DUX MESSAGE STRUCTURE*/
{
	dev_t dev;
	union {
		cdno_t cdno;
		ino_t ino;
	}un;
	int	fstype;
	int subtract;
};
/* Open request */
struct open_request		/*DUX MESSAGE STRUCTURE*/
{
	struct dux_lookup_request lookreq;
	int mode;
	int filemode;
};

/* Create request */
struct create_request		/*DUX MESSAGE STRUCTURE*/
{
	struct dux_lookup_request lookreq;
	struct vattr vattr;
	int mode;
	int filemode;
	enum vcexcl excl;
};

/* Common open/create reply */
struct copen_reply		/*DUX MESSAGE STRUCTURE*/
{
	struct remvno pkv;
	int openreplflags;
};

/* flags for open/create reply */
#define OPENSYNC	1	/* use syncrhonous I/O */
#define OPENDEVSYNC	2	/* synchronize disc */
#ifdef QUOTA
#define OPENDQOFF		4	/* at server quotas off for this fs */
#define OPENDQOVERSOFT		8	/* user is over file soft limit */
#define OPENDQOVERHARD		16	/* user is over file hard limit */
#define OPENDQOVERTIME		32	/* user is over file time limit */
#endif /* QUOTA */

/* Access request */
struct access_request		/*DUX MESSAGE STRUCTURE*/
{
	struct dux_lookup_request lookreq;
	u_int mode;
};

/* Set attributes request */
struct namesetattr_request	/*DUX MESSAGE STRUCTURE*/
{
	struct dux_lookup_request lookreq;
	struct vattr vattr;
	int null_time;
};

/* remove (unlink/rmdir) request */
struct remove_request		/*DUX MESSAGE STRUCTURE*/
{
	struct dux_lookup_request lookreq;
	enum rm dirflag;
};

/* link request */
struct link_request		/*DUX MESSAGE STRUCTURE*/
{
	struct dux_lookup_request lookreq;
	dev_t dev;
	ino_t ino;
	cdno_t cdno;
	int	fstype;
};

/* get mount device reply */
struct getmdev_reply		/*DUX MESSAGE STRUCTURE*/
{
	dev_t dev;
	site_t site;
};

/* lockf() or fcntl() file locking requst */
#ifdef F_SETLK		/* Only define if fcntl.h included */
struct lockf_req		/*DUX MESSAGE STRUCTURE*/
{
	dev_t  		lr_idev;
	ino_t  		lr_ino;
	int    		lr_flag;
	off_t  		lr_LB;
	off_t  		lr_UB;
	enum lockf_type lr_function;
	struct flock    lr_flock;
	int             lr_fpflags;
};

/* unlock everything on file close request */
struct unlockf_req		/*DUX MESSAGE STRUCTURE*/
{
	dev_t  		lr_idev;
	ino_t  		lr_ino;
};
#endif /* F_SETLK */


#ifdef	S_IFMT		/*only include if stat.h included*/
/* stat reply*/
struct stat_reply		/*DUX MESSAGE STRUCTURE*/
{
	struct stat stat;
	site_t my_site;		/*site where stat took place*/
};
#endif	/* S_IFMT */

#ifdef VFS_RDONLY	/* only include if vfs.h included */
/* statfs reply */
struct statfs_reply		/* DUX MESSAGE STRUCTURE */
{
	struct statfs statfs;
};
#endif /* VFS_RDONLY */

#ifdef ACLS

struct dux_acl_tuple 		/* DUX MESSAGE STRUCTURE */
{
	unsigned long uid;
	unsigned long gid;
	unsigned long mode;
};

struct setacl_request		/* DUX MESSAGE STRUCTURE */
{
	struct dux_lookup_request lookreq;
	int ntuples;
	struct dux_acl_tuple acl[NACLTUPLES];
};

struct getacl_request		/* DUX MESSAGE STRUCTURE */
{
	struct dux_lookup_request lookreq;
	int ntuples;
};

struct getacl_reply		/* DUX MESSAGE STRUCTURE */
{
	int tuple_count;
	struct dux_acl_tuple acl[NACLTUPLES];
};

struct getaccess_reply		/* DUX MESSAGE STRUCTURE */
{
	int mode;
};
#endif /* ACLS */
#ifdef	POSIX

struct pathconf_request
{
	struct dux_lookup_request lookreq;
	int name;
};
struct pathconf_reply
{
	int result;
};

#endif	/* POSIX */
/*close messages*/
struct closereq		/*DUX MESSAGE STRUCTURE*/
{
	dev_t dev;
	ino_t inumber;
	int flag;
	int open_error;
	int fifo_included;
	struct ic_fifo fifo;
	int timeflag;
	struct timeval atime;
	struct timeval ctime;
	struct timeval mtime;
	int	fstype;
	cdno_t cdnumber;
};

struct closerepl	/*DUX MESSAGE STRUCTURE*/
{
	int version;
};

/*block oriented asynchronous read or write (strategy) request*/

struct duxstratreq		/*DUX MESSAGE STRUCTURE*/
{
	dev_t ns_dev;		/*device*/
	union {
		ino_t ns_un_ino;		/*i number of file*/
		cdno_t ns_un_cdno;		/*cd number of file*/
	} ns_un;
	daddr_t ns_offset;	/*offset into file*/
	long ns_count;		/*number of bytes*/
	u_short ns_uid;		/*effective uid*/
	short ns_flags;		/*copied from bp->b_flags*/
	int	ns_fstype;	/*MOUNT_UFS, MOUNT_CDFS*/
};
#define	ns_ino ns_un.ns_un_ino
#define	ns_cdno ns_un.ns_un_cdno

/*block oriented asynchronous read or write (strategy) response*/

struct duxstratresp		/*DUX MESSAGE STRUCTURE*/
{
	unsigned int ns_resid;	/*words not transferred after error*/
	short ns_flags;		/* see below */
};

struct fsctl_request {
	int	fstype;
	int	command;
	dev_t	dev;
	union {
		ino_t inumber;
		cdno_t cdnumber;
	} un_fs;
	int	arg1;
};

struct fsctl_reply	/*DUX MESSAGE STRUCTURE*/
{
	int bytes_transfered;
};


#ifdef QUOTA
struct quota_reply	/*DUX MESSAGE STRUCTURE*/
{
        dev_t   rdev;       /* The real device number and site  */
        site_t  site;
        ushort dqreplflags; /* flags for quota info from server */
};
#define DQOFF                   1	/* quotas were off at time of msg */
#define DQOVERSOFT		2	/* user is over file soft limit */
#define DQOVERHARD		4	/* user is over file hard limit */
#define DQOVERTIME		8	/* over soft limit & time expired */
#endif /* QUOTA */
