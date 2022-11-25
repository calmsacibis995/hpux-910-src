
/* @(#)automount:	$Revision: 1.5.109.4 $	$Date: 92/05/06 13:57:31 $
*/

/*********************************************************************/
/* HP's version from nfs.h - pdb */
struct nfs_args {
        struct sockaddr_in      *addr;          /* file server address */
        fhandle_t               *fh;            /* File handle to be mounted */
        int                     flags;          /* flags */
        int                     wsize;          /* write size in bytes */
        int                     rsize;          /* read size in bytes */
        int                     timeo;          /* initial timeout in .1 secs */
        int                     retrans;        /* times to retry send */
        char                    *hostname;      /* server's name */
        int                     acregmin;       /* attr cache file min secs */
        int                     acregmax;       /* attr cache file max secs */
        int                     acdirmin;       /* attr cache dir min secs */
        int                     acdirmax;       /* attr cache dir max secs */
        char                    *fsname;        /* server's fs path name */
};

/*
 * NFS mount option flags
 */
#define NFSMNT_SOFT     0x001   /* soft mount (hard is default) */
#define NFSMNT_WSIZE    0x002   /* set write size */
#define NFSMNT_RSIZE    0x004   /* set read size */
#define NFSMNT_TIMEO    0x008   /* set initial timeout */
#define NFSMNT_RETRANS  0x010   /* set number of request retrys */
#define NFSMNT_HOSTNAME 0x020   /* set hostname for error printf */
#define NFSMNT_INT      0x040   /* set option to have interruptable mounts */
#define NFSMNT_NODEVS   0x080   /* turn off device file access (default on) */
#define NFSMNT_FSNAME   0x100   /* provide name of server's fs to system */
#define NFSMNT_IGNORE   0x200   /* mark this file system as ignore in mnttab */
#define NFSMNT_NOAC     0x400   /* don't cache file attributes */
#define NFSMNT_NOCTO    0x800   /* don't get new attributes on open */
#define NFSMNT_ACREGMIN 0x02000 /* set min secs for file attr cache */
#define NFSMNT_ACREGMAX 0x04000 /* set max secs for file attr cache */
#define NFSMNT_ACDIRMIN 0x08000 /* set min secs for dir attr cache */
#define NFSMNT_ACDIRMAX 0x10000 /* set max secs for dir attr cache */
