/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nfs/RCS/nfs_subr.c,v $
 * $Revision: 1.15.83.9 $	$Author: craig $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/10/20 16:01:27 $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../ufs/fsdir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../h/uio.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"
#include "../h/kern_sem.h"	/* MPNET */
#include "../net/netmp.h"	/* MPNET */
#ifdef AUDIT
#include "../h/audit.h"			/* auditing support */
#endif
#include "../h/syscall.h"		/* for nfs initialization */
#include "../h/kern_sem.h"

#include "../h/stat.h"
#ifdef	FSD_KI
#include "../h/ki_calls.h"
#endif	/* FSD_KI */

#ifdef hpux
#include "../h/mount.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"


/*
 * added for configurability -- gmf
 */
/*
 * this structure must always be present, even if DUX is not
 * configured in -- gmf.
 */
extern struct dm_funcentry dm_functions[];
/* these functions in nfs_vfsops.c */
extern nfs_umount_serve();
extern nfs_umount_commit();
extern struct vfs   *find_mntinfo();
extern enter_mntinfo();
extern delete_mntinfo();
/* these functions in nfs_server.c */
extern nfs_fcntl();
extern free_lock_with_lm();
extern clean_up_lm();

typedef int (*pfi)();

extern struct vfsops *vfssw[];
extern struct vfsops nfs_vfsops;
extern struct sysent sysent[];	/* system call table */
/*
 * functions for the table, sysent and their indices (in init_sent.c).
 */
extern int async_daemon();
extern int nfs_getfh();
extern int nfs_svc();
extern int exportfs();

/*
 * Pull in nfs_vers.o, to get the what string included there.  This allows
 * the what string to show up in the configured kernel.  [ Series 300 only ]
 */
#ifndef lint
char nfs_vers[];
#endif

/* end configure -- gmf */
#endif hpux


#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"

int setdomainname();



/*
 * the length of domainname must be 15 since file name length is limited to
 * 14 characters.  If we support long file names in the future we will want
 * to extend this to handle 64 characters (length=65)  as supported by
 * yellow pages. -- gmf, mds.
 */
extern	int 	domainnamelen;
extern	char	domainname[DOMAINNAMELENGTH];

#ifdef NFSDEBUG
int nfsdebug = 2;
#endif

/*
 * The string used to do logging for NFS and RPC in the kernel
 */
/* char 	sprintf_buf[SP_BUF_SIZE + 1]; */

extern struct vnodeops nfs_vnodeops;
struct inode *iget();
struct rnode *rfind();
struct vnode *makenfsnode();

/*
 * Client side utilities
 */

/*
 * client side statistics
 */
struct {
	int	nclsleeps;		/* client handle waits */
	int	nclgets;		/* client handle gets */
	int	ncalls;			/* client requests */
	int	nbadcalls;		/* rpc failures */
	int	reqs[32];		/* count of each request */
} clstat;

struct chtab {
	struct chtab *ch_next, *ch_prev;
	int	ch_timesused;
	bool_t	ch_inuse;
	CLIENT	*ch_client;
	int     ch_pid;
} chtabhead = { &chtabhead, &chtabhead, 0, 0, NULL, 0 };

u_int nchtotal;
u_int nchfree;
u_int nchtable=2;  /* keep at least this many CLIENT structures around */

#if defined(_WSIO) || defined(__hp9000s300)
/*
 * link routine for nfs.  Must be called at boot time if nfs is
 * configured into kernel.  This function should be enough to
 * dereference the rest of NFS for linking at configuration time.
 * It will be referenced by conf.c, created from /etc/master, which has
 * the nfsc line ifdef'd HPNFS. -- gmf
 */

int
nfsc_link()
#else /* hp9000s800 */
nfs_init()
#endif
{
#ifdef __hp9000s800
	extern int nfs_initialized;

	nfs_initialized = 1;
#endif
	/*
	 * set up the vfssw so the file system knows about NFS
	 */
	vfssw_assign(MOUNT_NFS, nfs_vfsops);
	/*
	 * replace defaults with real system call entry points for
	 * services.
	 */
	sysent_assign(SYS_NFSSVC,1, nfs_svc);
	sysent_assign(SYS_GETFH,2, nfs_getfh);
	sysent_assign(SYS_SETDOMAINNAME,2, setdomainname);
	sysent_assign(SYS_ASYNC_DAEMON,0, async_daemon);
	sysent_assign(SYS_NFS_FCNTL,3, nfs_fcntl);
	sysent_assign(SYS_EXPORTFS,2, exportfs);
	/*
	 * put entry points for unmounting in DUX table (called from
	 * send_nfs_unmount in nfs_vfsops.c).
	 */
	dm_functions[DM_NFS_UMOUNT].dm_callfunc = nfs_umount_serve;
	dm_functions[DM_COMMIT_NFS_UMOUNT].dm_callfunc = nfs_umount_commit;
	dm_functions[DM_ABORT_NFS_UMOUNT].dm_callfunc = nfs_umount_commit;

	/*
	 * nfsproc table initialization for indirect calls from DUX
	 * (calls from dux_mount.c and dux_lookup.c).
	 */
	nfsproc[NFS_RFIND] = (pfi) rfind;
	nfsproc[NFS_MAKENFSNODE] = (pfi) makenfsnode;
	nfsproc[NFS_FIND_MNT] = (pfi) find_mntinfo;
	nfsproc[NFS_ENTER_MNT] = enter_mntinfo;
	nfsproc[NFS_DELETE_MNT] = delete_mntinfo;
	nfsproc[NFS_LMEXIT] = clean_up_lm;
	nfsproc[NFS_INFORMLM] = free_lock_with_lm;
}

CLIENT *
clget(mi, cred, error_ret)
	struct mntinfo *mi;
	struct ucred *cred;
	int *error_ret;
{
	register struct chtab *ch;

	clstat.nclgets++;
	for (ch = chtabhead.ch_next; ch != &chtabhead; ch = ch->ch_next) {
		if (!ch->ch_inuse) {
			ch->ch_inuse = TRUE;
			clntkudp_init(ch->ch_client, &mi->mi_addr,
							mi->mi_retrans, cred);
			nchfree--;
			goto out;
		}
	}

        /*
         *  If we get here, we need to make a new handle
         */
        ch = (struct chtab *)kmem_alloc( sizeof (struct chtab));
	if (ch != NULL) {
		/* Create the CLIENT structure */
		ch->ch_client = clntkudp_create(&mi->mi_addr,
					    	NFS_PROGRAM, NFS_VERSION,
					    	mi->mi_retrans, cred);
	}

	if ((ch == NULL) || (ch->ch_client == NULL)) {
	        /* We return and not panic
	         * as the SUN code does.  This will
	         * cause ENOMEM to be given to the user.
	         */
		if (ch) kmem_free((caddr_t)ch, sizeof (struct chtab));
		NS_LOG(LE_NFS_ENOMEM, NS_LC_RESOURCELIM,NS_LS_NFS,0);
		*error_ret = ENOMEM;
		return(NULL);
	}

	ch->ch_inuse = TRUE;
	ch->ch_timesused = 0;

	/* Add it to the list */
	ch->ch_next = chtabhead.ch_next;
	ch->ch_prev = &chtabhead;
	chtabhead.ch_next->ch_prev = ch;
	chtabhead.ch_next = ch;
	nchtotal++;

out:
	ch->ch_pid = (int) u.u_procp->p_pid;
	ch->ch_timesused++;
	return (ch->ch_client);
}

clfree(cl)
	CLIENT *cl;
{
	register struct chtab *ch;
	extern int async_daemon_count;

	for (ch = chtabhead.ch_next; ch != &chtabhead; ch = ch->ch_next) {
		if (ch->ch_client == cl) {
			/*
			 * Free up the credential structure grabbed by
			 * clntkudp_init() --- dds --- 6/2/87
			 */
			clntkudp_freecred(cl);
			/* Since the BIOD's will be using client handles
			 * quite often, make sure we keep enough handles
			 * around for each BIOD.  cwb
			 */
			if (nchfree >= (nchtable + async_daemon_count)) {
				/*
                                 *  We have enough free client handles,  so
                                 *  we can simply pop this one back into the
                                 *  heap where someone else can use the space.
                                 */
                                ch->ch_prev->ch_next = ch->ch_next;
                                ch->ch_next->ch_prev = ch->ch_prev;
				/* This space is malloc'd, get rid of it
				 * before we delete what points to it.
				 * A fix from COSL.
				 */
				AUTH_DESTROY(cl->cl_auth);
                                CLNT_DESTROY(cl);
                                kmem_free((caddr_t)ch, sizeof (struct chtab));
				nchtotal--;
			}
			else {
				/*
				 *  We don't have enough free client handles,
				 *  so leave this one in the list.
				 */
				ch->ch_inuse = FALSE;
				nchfree++;
			}
			return;
		}
	}
}

char *rpcstatnames[] = {
	"SUCCESS", "CANT ENCODE ARGS", "CANT DECODE RES", "CANT SEND",
	"CANT RECV", "TIMED OUT", "VERS MISMATCH", "AUTH ERROR", "PROG UNAVAIL",
	"PROG VERS MISMATCH", "PROC UNAVAIL", "CANT DECODE ARGS","SYSTEM ERROR",
	"UNKNOWN HOST", "PMAP FAILURE", "PROG NOT REGISTERED", "FAILED",
	"UNKNOWN", "INTERRUPTED"
	};
/* HPNFS New code to add interruptable option in the kernel */


char *rfsnames[] = {
	"null", "getattr", "setattr", "unused", "lookup", "readlink", "read",
	"unused", "write", "create", "remove", "rename", "link", "symlink",
	"mkdir", "rmdir", "readdir", "fsstat" };

/*
 * Back off for retransmission timeout, MAXTIMO is in 10ths of a sec
 */
#define MAXTIMO	300
#define backoff(tim)	((((tim) << 2) > MAXTIMO) ? MAXTIMO : ((tim) << 2))

int
rfscall(mi, which, xdrargs, argsp, xdrres, resp, cred)
	register struct mntinfo *mi;
	int	 which;
	xdrproc_t xdrargs;
	caddr_t	argsp;
	xdrproc_t xdrres;
	caddr_t	resp;
	struct ucred *cred;
{
	CLIENT *client;
	register enum clnt_stat status;
	struct rpc_err rpcerr;
	struct timeval wait;
	struct ucred *newcred;
	int timeo;
	int user_told = 0;
	bool_t tryagain;
	int error_ret, oldlevel;
#ifdef	MP
	sv_sema_t savestate;
#endif
#ifdef	FSD_KI
	struct ki_timeval starttime;
#endif	/* FSD_KI */
	int vhand = (u.u_procp->p_pid == PID_PAGEOUT);


	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "rfscall: %x, %d, %x, %x, %x, %x\n",
	    mi, which, xdrargs, argsp, xdrres, resp);
#endif
	clstat.ncalls++;
	clstat.reqs[which]++;

	rpcerr.re_errno = 0;
	newcred = NULL;
	timeo = mi->mi_timeo;
#ifdef	FSD_KI
	/* stamp bp with enqueue time */
	KI_getprectime(&starttime);
#endif	/* FSD_KI */
retry:
	if ( (client = clget(mi, cred, &error_ret)) == NULL) {
 	/* Could not get a client, the reason is in error_ret*/
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
 		return(error_ret);
 	}
	/*
	 * If hard mounted fs, retry call forever unless hard error occurs
	 */
	do {
		tryagain = FALSE;

		wait.tv_sec = timeo / 10;
		wait.tv_usec = 100000 * (timeo % 10);
		status = CLNT_CALL(client, which, xdrargs, argsp,
		    xdrres, resp, wait);
		/*
		 * HPNFS
		 * Because of differing errno value, we need to translate
		 * the return value of the remote call.
		 */
		if ( ((struct gen_result *) resp)->gen_status)
			((struct gen_result *)resp)->gen_status = (enum nfsstat)
			   nfs_to_hp_errors( (int) (((struct gen_result *) resp)->gen_status));

		switch (status) {
		case RPC_SUCCESS:
#ifdef	FSD_KI
			KI_rfscall(mi, which, argsp, resp, &starttime);
#endif	/* FSD_KI */
			break;

		/*
		 * Unrecoverable errors: give up immediately
		 */
		case RPC_AUTHERROR:
		case RPC_CANTENCODEARGS:
		case RPC_CANTDECODERES:
		case RPC_VERSMISMATCH:
		case RPC_PROGVERSMISMATCH:
		case RPC_CANTDECODEARGS:
		case RPC_PROCUNAVAIL:
			break;

		case RPC_INTR:
			rpcerr.re_status = RPC_SYSTEMERROR;
			rpcerr.re_errno = EINTR;
			tryagain = FALSE;
			break;

		default:
			if (!mi->mi_printed) {
				mi->mi_printed = 1;
				if (mi->mi_hard) {
					NS_LOG_STR(LE_NFS_SERVER_TRYING,
					NS_LC_WARNING,NS_LS_NFS,0,
					mi->mi_hostname);
				} else {
					NS_LOG_STR(LE_NFS_SERVER_GIVE_UP,
					NS_LC_WARNING,NS_LS_NFS,0,
					mi->mi_hostname);
				}
			}
                        /*
                         * After great debate, it was decided to put the
                         * printing of the message back into the 300
                         * But it will be printed only after the second
                         * NFS timeout.  I realize that this code will cause
                         * user_told to wrap around back to 2, but by then
                         * it might be time to tell the user again.
                         * This should prevent the user from being bothered
                         * by the message since he should only get it when
                         * the server is really down and by then that will
                         * be the least of his worries.   mds   11/03/88
                         */
                        if (!mi->mi_hard) {
                                uprintf(
                                  "NFS server %s not responding, giving up\n",
                                      mi->mi_hostname);
                        } else if (++user_told == 2) {
                                uprintf(
                                "NFS server %s not responding, still trying\n",
                                      mi->mi_hostname);
                        }

			/*
			 * This server is "down" at the moment.  Record
			 * this fact.  If we are vhand, we also setup a
			 * timer to tell ourselves when we can try
			 * again.
			 */
			mi->mi_down = 1;
			if (vhand) {
				extern long vhand_nfs_retry;

				vhand_nfs_retry = 90; /* seconds */
			}

			/*
			 * HPNFS:  Enable interrupts only AFTER at least one
			 * timeout.  The basic idea here is to insure we have
			 * at least SOME time to complete a transaction with
			 * the server.  This is a pseudo work-around for those
			 * sections of code that don't handle interrupts well.
			 * E.g. we don't want an interrupt during page-in, but
			 * we don't want hung processes either.
			 */
			if (mi->mi_int)
			    clntkudp_setint(client);

			tryagain = TRUE;
			timeo = backoff(timeo);
		}
	} while (tryagain && mi->mi_hard && !vhand);

	if (status != RPC_SUCCESS) {
		char    spare_buffer[200];
		CLNT_GETERR(client, &rpcerr);
		clstat.nbadcalls++;
		sprintf(spare_buffer,200,"%s %s %s",
			    	mi->mi_hostname,rfsnames[which],
				rpcstatnames[(int)status]);
		if (status == RPC_INTR) {
			NS_LOG_STR(LE_NFS_FUNC_FAIL_WARN,NS_LC_WARNING,
				NS_LS_NFS,0, spare_buffer);
		} else {
			NS_LOG_STR(LE_NFS_FUNC_FAIL_ERROR,NS_LC_ERROR,
				NS_LS_NFS,0, spare_buffer);
		}
	} else if (resp && *(int *)resp == EACCES &&
	    newcred == NULL && cred->cr_uid == 0 && cred->cr_ruid != 0) {
		/*
		 * Boy is this a kludge!  If the reply status is EACCES
		 * it may be because we are root (no root net access).
		 * Check the real uid, if it isn't root make that
		 * the uid instead and retry the call.
		 */
		newcred = crdup(cred);
		cred = newcred;
		cred->cr_uid = cred->cr_ruid;
		clfree(client);
		goto retry;
	} else if (mi->mi_hard) {
		mi->mi_down = 0;
		if (mi->mi_printed) {
			NS_LOG_STR(LE_NFS_SERVER_OK,NS_LC_WARNING,NS_LS_NFS,
			    0,mi->mi_hostname);
			mi->mi_printed = 0;
		}
		if (user_told >= 2) {
			uprintf("NFS server %s ok\n", mi->mi_hostname);
		}
	} else {
		mi->mi_down = 0;
	}

	clfree(client);
#ifdef NFSDEBUG
	dprint(nfsdebug, 7, "rfscall: returning %d\n", rpcerr.re_errno);
#endif
	if (newcred) {
		crfree(newcred);
	}
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */
	return (rpcerr.re_errno);
}

#ifdef notneeded
/* HPNFS
 * We changed the way that we handle interrupts, so this routine from
 * SUN is not needed anymore.  I have it here in the source just so we
 * can see that it was removed when we try to merge our changes to the
 * next version of NFS.   mds   CND 09/18/87
 */

/*
 * Check if this process got an interrupt from the keyboard while sleeping
 */
int
interrupted()
{
	int s, smask, intr;
	extern wakeup();
	struct proc *p = u.u_procp;

#define bit(a) 	(1<<(a-1))

	s = spl6();
	smask = p->p_sigmask;
	p->p_sigmask |=
		~(bit(SIGHUP) | bit(SIGINT) | bit(SIGQUIT) | bit(SIGTERM) |
		  bit(SIGKILL));  /* HPNFS Added the SIGKILL bit */

	if (ISSIG(u.u_procp)) {
		intr = TRUE;
	} else {
		intr = FALSE;
	}
	p->p_sigmask = smask;
	(void) splx(s);

	return(intr);
}
#endif notneeded

/*HPNFS Now code for interrupts*/


nattr_to_vattr(na, vap)
	register struct nfsfattr *na;
	register struct vattr *vap;
{

	vap->va_type = (enum vtype)na->na_type;
	vap->va_mode = na->na_mode;
	vap->va_uid = na->na_uid;
	vap->va_gid = na->na_gid;
	vap->va_fsid = na->na_fsid;
	vap->va_nodeid = na->na_nodeid;
	vap->va_nlink = na->na_nlink;
	vap->va_size = na->na_size;
	vap->va_atime.tv_sec  = na->na_atime.tv_sec;
	vap->va_atime.tv_usec = na->na_atime.tv_usec;
	vap->va_mtime.tv_sec  = na->na_mtime.tv_sec;
	vap->va_mtime.tv_usec = na->na_mtime.tv_usec;
	vap->va_ctime.tv_sec  = na->na_ctime.tv_sec;
	vap->va_ctime.tv_usec = na->na_ctime.tv_usec;
	vap->va_rdev = na->na_rdev;
	vap->va_blocks = na->na_blocks;
	vap->va_fstype = MOUNT_NFS;
	/*
	 * The following fields make no sense to NFS, so set them to zero
	 * so that the values are always consistent.
	 */
	vap->va_fssite = 0;
	vap->va_rsite = 0;
	/*
	 * Set the realdev equal to the other dev, since that's the best
	 * we can do.
	 */
	vap->va_realdev = na->na_rdev;
#ifdef ACLS
	/*
	 * With NFS, we don't support ACLS, so set the basemode to be
	 * the same as the mode, since we don't know what else to do with it.
	 */
	vap->va_acl = 0;
	vap->va_basemode = vap->va_mode;
#endif ACLS
	switch(na->na_type) {

	case NFBLK:
		vap->va_blocksize = BLKDEV_IOSIZE;
		break;

	case NFCHR:
		vap->va_blocksize = MAXBSIZE;
		break;

	default:
		vap->va_blocksize = na->na_blocksize;
		break;
	}
	/*
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes.  It remaps the
	 * special over-the-wire type to the VFIFO type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-SUN server, you probably
	 *  don't want to include the following block of code.  The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if (NA_ISFIFO(na)) {
		vap->va_type = VFIFO;
		vap->va_mode = (vap->va_mode & ~S_IFMT) | S_IFIFO;
		vap->va_rdev = 0;
		vap->va_blocksize = na->na_blocksize;
	}

}

vattr_to_sattr(vap, sa)
	register struct vattr *vap;
	register struct nfssattr *sa;
{
	register int uid;
	
	if ((int) (short) (vap->va_mode) == -1)
	  	sa->sa_mode = (u_long) (-1L);
	else
	  	sa->sa_mode = vap->va_mode;
	if ((uid = (int) (short) (vap->va_uid)) == -1 || uid == -2)
		sa->sa_uid = (u_long) (long) uid;
	else
		sa->sa_uid = vap->va_uid;
	if ((int) (short) (vap->va_gid) == -1)
	  	sa->sa_gid = (u_long) (-1L);
	else
	  	sa->sa_gid = vap->va_gid;

	sa->sa_size = vap->va_size;
	sa->sa_atime.tv_sec  = vap->va_atime.tv_sec;
	sa->sa_atime.tv_usec = vap->va_atime.tv_usec;
	sa->sa_mtime.tv_sec  = vap->va_mtime.tv_sec;
	sa->sa_mtime.tv_usec = vap->va_mtime.tv_usec;
}

setdiropargs(da, nm, dvp)
	struct nfsdiropargs *da;
	char *nm;
	struct vnode *dvp;
{

	da->da_fhandle = *vtofh(dvp);
	da->da_name = nm;
}

struct rnode *rpfreelist = NULL;
int rreuse, rnew, ractive;


/*
 * return a vnode for the given fhandle.
 * If no rnode exists for this fhandle create one and put it
 * in a table hashed by fh_fsid and fs_fid.  If the rnode for
 * this fhandle is already in the table return it (ref count is
 * incremented by rfind.  The rnode will be flushed from the
 * table when nfs_inactive calls runsave.
 */
struct vnode *
makenfsnode(fh, attr, vfsp)
	fhandle_t *fh;
	struct nfsfattr *attr;
	struct vfs *vfsp;
{
	register struct rnode *rp;
	char newnode = 0;

	if ((rp = rfind(fh, vfsp)) == NULL) {
		if (rpfreelist) {
			rp = rpfreelist;
			rpfreelist = rp->r_next;
			rreuse++;
		} else {
			/* unpack_vfs() can call this under interrupt
			 * during dux client initialization. Otherwise
			 * M_WAITOK is fine. Since we are normally
			 * NOT on the istack and therefore call
			 * with M_WAITOK which will not fail, panic
			 * on failure.
			 */
                        MALLOC(rp, struct rnode *, sizeof(struct rnode), M_DYNAMIC,ON_ISTACK?M_NOWAIT:M_WAITOK);
                       if (rp == NULL)
			       panic("makenfsnode: no memory available");
			rnew++;
		}
		bzero((caddr_t)rp, sizeof(*rp));
		rp->r_fh = *fh;
		rtov(rp)->v_count = 1;
		rtov(rp)->v_op = &nfs_vnodeops;
		rtov(rp)->v_fstype = VNFS;
                if (attr) {
                        rtov(rp)->v_type = n2v_type(attr);
                        rtov(rp)->v_rdev = n2v_rdev(attr);
                }
		rtov(rp)->v_data = (caddr_t)rp;
		rtov(rp)->v_vfsp = vfsp;
		rsave(rp);
		((struct mntinfo *)(vfsp->vfs_data))->mi_refct++;
		newnode++;
	}
	if (attr) {
		nfs_attrcache(rtov(rp), attr, (newnode? NOFLUSH: SFLUSH), u.u_cred);
	}
	return (rtov(rp));
}

/*
 * Rnode lookup stuff.
 * These routines maintain a table of rnodes hashed by fhandle so
 * that the rnode for an fhandle can be found if it already exists.
 * NOTE: RTABLESIZE must be a power of 2 for rtablehash to work!
 *
 * NFS 3.2 NOTE:  Because the inode is no somewhat hidden from the NFS layer,
 * the hashing algorithm changed.  fh_data[2] and fh_data[5] will pick out
 * values from the fsid, which on HP systems will be the minor number and
 * MOUNT_UFS.  The fh_data[15] will align with the last byte of the inode
 * for UFS systems, which should give a fairly good distribution.
 */

#define	RTABLESIZE	16
#define	rtablehash(fh) \
    ((fh->fh_data[2] ^ fh->fh_data[5] ^ fh->fh_data[15]) & (RTABLESIZE-1))

struct rnode *rtable[RTABLESIZE];

/*
 * Put a rnode in the table
 */
rsave(rp)
	struct rnode *rp;
{

	rp->r_next = rtable[rtablehash(rtofh(rp))];
	rtable[rtablehash(rtofh(rp))] = rp;
}

/*
 * Remove a rnode from the table
 */
runsave(rp)
	struct rnode *rp;
{
	struct rnode *rt;
	struct rnode *rtprev = NULL;

	rt = rtable[rtablehash(rtofh(rp))];
	while (rt != NULL) {
		if (rt == rp) {
			if (rtprev == NULL) {
				rtable[rtablehash(rtofh(rp))] = rt->r_next;
			} else {
				rtprev->r_next = rt->r_next;
			}
			return;
		}	
		rtprev = rt;
		rt = rt->r_next;
	}	
}

/*
 * Put an rnode on the free list and take it out of
 * the hash table.
 */
rfree(rp)
	register struct rnode *rp;
{

   /*
    * rp is already off the hash list when rfree is called from
    * nfs_inactive().
    */
   /*	runsave(rp); */
	rp->r_next = rpfreelist;
	rpfreelist = rp;
}

/*
 * Lookup a rnode by fhandle
 */
struct rnode *
rfind(fh, vfsp)
	fhandle_t *fh;
	struct vfs *vfsp;
{
	register struct rnode *rt;

	rt = rtable[rtablehash(fh)];
	while (rt != NULL) {
		if (bcmp((caddr_t)rtofh(rt), (caddr_t)fh, sizeof(*fh)) == 0 &&
		    vfsp == rtov(rt)->v_vfsp) {
			SPINLOCK(v_count_lock);
			rtov(rt)->v_count++;
			SPINUNLOCK(v_count_lock);
			ractive++;
			return (rt);
		}	
		rt = rt->r_next;
	}	
	return (NULL);
}

/*
 * remove buffers from the buffer cache that have this vfs.
 * NOTE: assumes buffers have been written already.
 */
rinval(vfsp)
	struct vfs *vfsp;
{
	int	i;
	register struct rnode *rp, *rptmp;

	for (i = 0; i < RTABLESIZE; i++) {
		for (rp = rtable[i]; rp; ) {
			if (rp->r_vnode.v_vfsp == vfsp) {
				/*
				 * Need to hold the vp because binval releases
				 * it which will call rfree if the ref count
				 * goes to zero and rfree will mess up rtable.
				 */
				VN_HOLD(rtov(rp));
				rptmp = rp;
				rp = rp->r_next;
				binvalfree(rtov(rptmp));
				VN_RELE(rtov(rptmp));
			} else {
				rp = rp->r_next;
			}
		}
	}
}

/*
 * Flush dirty buffers for all vnodes in this vfs
 */
rflush(vfsp)
	struct vfs *vfsp;
{
	int	i;
	register struct rnode *rp;

	for (i = 0; i < RTABLESIZE; i++) {
		for (rp = rtable[i]; rp; rp = rp->r_next) {
			if (rp->r_vnode.v_vfsp == vfsp) {
				bflush(rtov(rp));
			}
		}
	}
}

#define	PREFIXLEN	4
static char prefix[PREFIXLEN+1] = ".nfs";

char *
newname()
{
	char *news;
	register char *s1, *s2;
	int id;
	static int newnum = 0;

	news = (char *)kmem_alloc((u_int)NFS_MAXNAMLEN);
	for (s1 = news, s2 = prefix; s2 < &prefix[PREFIXLEN]; ) {
		*s1++ = *s2++;
	}
	if ( newnum == 0 ) {
		newnum = time.tv_sec & 0xffff;
	}
	id = newnum++;
	while (id) {
		*s1++ = "0123456789ABCDEF"[id & 0x0f];
		id = id >> 4;
	}
	*s1 = '\0';
	return (news);
}


/*
 * Server side utilities
 */

vattr_to_nattr(vap, na)
	register struct vattr *vap;
	register struct nfsfattr *na;
{
	na->na_type = (enum nfsftype)vap->va_type;
	na->na_mode = vap->va_mode;
	na->na_uid = vap->va_uid;
	na->na_gid = vap->va_gid;
	na->na_fsid = vap->va_fsid;
	na->na_nodeid = vap->va_nodeid;
	na->na_nlink = vap->va_nlink;
	na->na_size = vap->va_size;
	na->na_atime.tv_sec  = vap->va_atime.tv_sec;
	na->na_atime.tv_usec = vap->va_atime.tv_usec;
	na->na_mtime.tv_sec  = vap->va_mtime.tv_sec;
	na->na_mtime.tv_usec = vap->va_mtime.tv_usec;
	na->na_ctime.tv_sec  = vap->va_ctime.tv_sec;
	na->na_ctime.tv_usec = vap->va_ctime.tv_usec;
	na->na_rdev = vap->va_rdev;
	na->na_blocks = vap->va_blocks;
	na->na_blocksize = vap->va_blocksize;

        /*
         * This bit of ugliness is a *TEMPORARY* hack to preserve the
         * over-the-wire protocols for named-pipe vnodes.  It remaps the
         * VFIFO type to the special over-the-wire type. (see note in nfs.h)
         *
         * BUYER BEWARE:
         *  If you are porting the NFS to a non-SUN server, you probably
         *  don't want to include the following block of code.  The
         *  over-the-wire special file types will be changing with the
         *  NFS Protocol Revision.
         */
        if (vap->va_type == VFIFO)
                NA_SETFIFO(na);
}

sattr_to_vattr(sa, vap)
	register struct nfssattr *sa;
	register struct vattr *vap;
{
	vattr_null(vap);
	vap->va_mode = sa->sa_mode;
	vap->va_uid = sa->sa_uid;
	vap->va_gid = sa->sa_gid;
	vap->va_size = sa->sa_size;
	vap->va_atime.tv_sec  = sa->sa_atime.tv_sec;
	vap->va_atime.tv_usec = sa->sa_atime.tv_usec;
	vap->va_mtime.tv_sec  = sa->sa_mtime.tv_sec;
	vap->va_mtime.tv_usec = sa->sa_mtime.tv_usec;
}


/*
 * General utilities
 */

/*
 * Returns the prefered transfer size in bytes based on
 * what network interfaces are available.
 */
nfstsize()
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_name[0] == 'e' && ifp->if_name[1] == 'c') {
#ifdef NFSDEBUG
			dprint(nfsdebug, 3, "nfstsize: %d\n", ECTSIZE);
#endif
			return (ECTSIZE);
		}
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 3, "nfstsize: %d\n", IETSIZE);
#endif
	return (IETSIZE);
}


/* HPNFS
 * These routines are used to convert the error return values
 * that are used by NFS to values used by HP-UX.  The problem is
 * the mapping of constants from errno.h to constants defined in
 * nfs.h to give the NFSERR_* values.  The cause of problems is
 * that SUN's errno values are different then HP's.  This
 * only happens for a few values, so it is not hard to implement, but
 * a pain to have to do it.
 */

#define SUN_EOPNOTSUPP    45
#define SUN_ENAMETOOLONG  63
#define SUN_ENOTEMPTY     66
#define SUN_EDQUOT        69

nfs_to_hp_errors(error)
	int error;
{
	if (error == SUN_ENAMETOOLONG)
		return(ENAMETOOLONG);
	if (error == SUN_ENOTEMPTY)
		return(ENOTEMPTY);
	if (error == SUN_EDQUOT)
		return(ENOSPC);
	if (error == SUN_EOPNOTSUPP)
		return(EOPNOTSUPP);
	return(error);
}

hp_to_nfs_errors(error)
	int error;
{
	if (error == ENAMETOOLONG)
		return(SUN_ENAMETOOLONG);
	if (error == ENOTEMPTY)
		return(SUN_ENOTEMPTY);
	if (error == EOPNOTSUPP)
		return(SUN_EOPNOTSUPP);
	return(error);
}


/*
 * This code was moved here from kern_xxx.c for configurability as
 * recommended by ISO command group.  See kern_xxx.c.
 * However configurability is done, it seems that setdomainname should
 * NOT be in kern_xxx.c
 */

/*
 * This is in the nfs code because ISO was worried that someone could
 * use it to set a domainname and cause our code to think that YP and
 * networking was installed and cause a program to crash trying to
 * access the net without the proper networking code installed.
 * So getdomainname() remains in sys/kern_xxx.c and setdomainname()
 * is here.   Mike Shipley  05/28/87
 */

setdomainname()
{
	register struct a {
		char	*domainname;
		u_int	len;
	} *uap = (struct a *)u.u_ap;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_str(uap->domainname);
	}
#endif /* AUDIT */

	if (!suser())
		return;
	if (uap->len > sizeof (domainname) - 1) {
		u.u_error = EINVAL;
		return;
	}
	domainnamelen = uap->len;
	u.u_error = copyin((caddr_t)uap->domainname, domainname, uap->len);
	domainname[domainnamelen] = 0;
}

#ifdef NFSDEBUG
/*
 * Utilities used by both client and server
 * Standard levels:
 * 0) no debugging
 * 1) hard failures
 * 2) soft failures
 * 3) current test software
 * 4) main procedure entry points
 * 5) main procedure exit points
 * 6) utility procedure entry points
 * 7) utility procedure exit points
 * 8) obscure procedure entry points
 * 9) obscure procedure exit points
 * 10) random stuff
 * 11) all <= 1
 * 12) all <= 2
 * 13) all <= 3
 * ...
 */

/*VARARGS2*/
dprint(var, level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
	int var;
	int level;
	char *str;
	int a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
	if (var == level || (var > 10 && (var - 10) >= level))
		printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}
#endif
