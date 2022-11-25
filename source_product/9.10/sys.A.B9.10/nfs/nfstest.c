/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nfs/RCS/nfstest.c,v $
 * $Revision: 1.5.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 19:09:02 $
 */


/* NFSSRC @(#)nfstest.c	2.2 86/08/15 */
/*      @(#)nfstest.c 1.1 86/08/03 VH      */


#include "../h/param.h"
#include "../h/systm.h"
#include "../ufs/fsdir.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/pathname.h"
#include "../h/uio.h"
#include "../h/socket.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"
#include "../h/mount.h"


#ifdef NFSDEBUG
extern int nfsdebug;
#endif

struct vnode *makenfsnode();
int nfsmntno;
int loopcount;

/*
 * nfs vfs operations.
 */
int nfstest0_open();
int nfstest0_ioctl();
int nfstest0_read();
int nfstest0_write();
int nfstest0_mkmnt();
int nfstest0_rmmnt();
extern int nullfree();
extern int xdr_void();


struct mntinfo *mnttmp = NULL;	/* mount info - temporary allocated */


/*ARGSUSED*/
nfstest0_open(nfs_dev_t, flag)
int    nfs_dev_t;
int    flag;
{
       return(0);
}

#include "../sio/netio.h"
#define NFSTEST_MKMNT NETCTRL
#define NFSTEST_RMMNT NETSTAT

/*ARGSUSED*/
nfstest0_ioctl(dev, cmd, data, flag)
int    dev;
int    cmd;
caddr_t data;
int    flag;
{
int    error;
    switch (cmd) {
    case NFSTEST_MKMNT: 
	    error = nfstest0_mkmnt(data);
            break;

    case NFSTEST_RMMNT: 
	    error = nfstest0_rmmnt(data);
            break;

    default:
	 return(EINVAL);
    }

    return(error);
}


/*
 * Set up mount info record 
 */
/*ARGSUSED*/
nfstest0_mkmnt(data)
	caddr_t data;
{
	int error = 0;
	struct mntinfo *mi = NULL;	/* mount info, pointed at by vfs */
	struct nfs_args args;		/* nfs mount arguments */
	struct fis      arg;            /* ioctl arguments     */

	/*
	 * get arguments
	 */
        bcopy(data, (caddr_t) &args, sizeof(args));
        bcopy(data, (caddr_t) &arg, sizeof(arg));

	/*
	 * create a mount record and link it to the vfs struct
	 */
	mi = (struct mntinfo *)kmem_alloc((u_int)sizeof(*mi));
	mi->mi_refct = 0;
	mi->mi_stsize = 0;
	mi->mi_hard = ((args.flags & NFSMNT_SOFT) == 0);
	if (args.flags & NFSMNT_RETRANS) {
		mi->mi_retrans = args.retrans;
		if (args.retrans < 0) {
			error = EINVAL;
			goto errout;
		}
	} else {
		mi->mi_retrans = NFS_RETRIES;
	}
	if (args.flags & NFSMNT_TIMEO) {
		mi->mi_timeo = args.timeo;
		if (args.timeo <= 0) {
			error = EINVAL;
			goto errout;
		}
	} else {
		mi->mi_timeo = NFS_TIMEO;
	}
	mi->mi_mntno = nfsmntno++;
	mi->mi_printed = 0;

	/* assign arg.vtype to loopcount - number of loopback pkt */

	loopcount = arg.vtype;

        if (loopcount <=0) loopcount =1;  /* default to 1 */

        /* copy the destination IP address from ioctl arg.reqtype
	   to mntinfo
        */

	bcopy(args, (caddr_t)&(mi->mi_addr.sin_addr),
	    sizeof(mi->mi_addr.sin_addr));

        mi->mi_addr.sin_family = AF_INET;
        mi->mi_addr.sin_port = NFS_PORT;

	if (args.flags & NFSMNT_HOSTNAME) {
		bcopy((caddr_t)args.hostname, (caddr_t)mi->mi_hostname,
		    HOSTNAMESZ);
		if (error) {
			goto errout;
		}
	} else {
		addr_to_str(&(mi->mi_addr), mi->mi_hostname);
	}


errout:
	if (error) {
		if (mi) {
			kmem_free((caddr_t)mi, (u_int)sizeof(*mi));
		}
		return(error);
	}
	mnttmp = mi;
	return (error);
}

/*
 * Free up mount info record 
 */
/*ARGSUSED*/
nfstest0_rmmnt(data)
	caddr_t data;
{
	struct mntinfo *mi = mnttmp;	/* mount info, pointed at by vfs */

	if (mi) {
		kmem_free((caddr_t)mi, (u_int)sizeof(*mi));
		}
	return 0;
}

/*ARGSUSED*/
nfstest0_read(dev, uio)
int    dev;
struct uio *uio;
{
     return(0);
}


/*ARGSUSED*/
nfstest0_write(dev, uio)
int    dev;
struct uio *uio;
{
int    error, i;
     /* call rfscall to send RFS_NULL pkt loopcount times */
     for (i=loopcount; i; i--) {
        error = rfscall(mnttmp, RFS_NULL, xdr_void, 0, xdr_void, 0, nullfree);
     }
     return(error);
}

/*ARGSUSED*/
nfstest0_close(dev)
int    dev;
{
     return(0);
}
