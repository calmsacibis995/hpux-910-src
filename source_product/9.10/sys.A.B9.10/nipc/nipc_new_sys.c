/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_new_sys.c,v $
 * $Revision: 1.2.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 19:10:58 $
 */

/* 
 * Netipc system call stubs.
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/time.h"
#include "../h/uio.h"
#include "../h/malloc.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/ns_ipc.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../nipc/nipc_cb.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_protosw.h"
#include "../nipc/nipc_domain.h"
#include "../nipc/nipc_err.h"
#include "../nipc/nipc_name.h"
#include "../nipc/nipc_hpdsn.h"
#if defined (TRUX) && defined (B1)
#include "../h/security.h"
#endif  /* TRUX && B1 */
#ifdef AUDIT
#include "../h/audit.h"
#endif /* AUDIT */


void
new_ipcconnect()
{
	 struct a {
		int	calldesc;	/* CALL SOCKET IS IGNORED */
		int	destdesc;	/* POINTER TO A 'DEST DESC' */
		caddr_t flags;
		caddr_t opt;
		caddr_t vcdesc;
		caddr_t	result;
	} *uap = (struct a *)u.u_ap;
	int result;

	result = ipcconnect();
	u.u_error = copyout(&result, uap->result, sizeof(int));
} 
 
void
new_ipccontrol()
{
	struct a {
		int	descriptor;
		int	request;
		caddr_t	wrtdata;
		int	wlen;
		caddr_t	readdata;
		caddr_t	rlen;
		caddr_t	flags;
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipccontrol();
	u.u_error = copyout(&result, uap->result, sizeof(int));
}

 
void
new_ipccreate()
{
	struct a {
		int	socketkind;
		int	protocol;	
		caddr_t	flags;	
		caddr_t	opt;
		caddr_t	calldesc;		/* OUTPUT */
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipccreate();
	u.u_error = copyout(&result, uap->result, sizeof(int));
} 
		
 

void
new_ipcdest()
{
	/* set up pointers to caller's parameters */
	 struct a {
		int	socketkind;	/* nipc socket kind of dest socket    */
		caddr_t nodename;	/* nodename where dest socket located */
		int	nodelen;	/* byte length of nodename            */
		int	protocol;       /* L4 protocol attached to dest socket*/
		caddr_t protoaddr;	/* protocol port address for dest sock*/
		int	protolen;	/* byte length of protoaddr           */
		caddr_t flags;		/* no flags are supported             */
		caddr_t opt;		/* options parm (no options defined   */
		caddr_t destdesc;	/* location for output dd parm        */
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipcdest();
	u.u_error = copyout(&result, uap->result, sizeof(int));
}  


void
new_ipcgetnodename()

{
	/* set up pointers to caller's parameters */
	 struct a {
		caddr_t nodename;	/* buffer to copy nodename to     */
		caddr_t	size;		/* byte length of nodename buffer */
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipcgetnodename();
	u.u_error = copyout(&result, uap->result, sizeof(int));
}  /* getnodename */


void
new_ipclookup()
{
	/* set up pointers to caller's parameters */
	 struct a {
		caddr_t	socketname;	/* name of socket to lookup           */
		int	nlen;		/* length of socketname		      */
		caddr_t nodename;	/* nodename where named socket located*/
		int	nodelen;	/* byte length of nodename            */
		caddr_t flags;		/* flags parm (no flags defined)      */
		caddr_t destdesc;	/* (output) destination descriptor    */
		caddr_t	protocol;       /* (output) protocol of named socket  */
		caddr_t socketkind;	/* (output) nipc type of named socket */
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipclookup();
	u.u_error = copyout(&result, uap->result, sizeof(int));
}  



void
new_ipcname()
{
	/* set up pointers to caller's parameters */
	 struct a {
		int 	descriptor;	/* file descriptor to be named */
		caddr_t socketname;	/* name to bind to descriptor  */
		int	nlen;		/* byte length of socketname   */
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipcname();
	u.u_error = copyout(&result, uap->result, sizeof(int));
}  


void
new_ipcnamerase()
{
	/* set up pointers to caller's parameters */
	 struct a {
		caddr_t socketname;	/* name to bind to descriptor  */
		int	nlen;		/* byte length of socketname   */
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipcnamerase();
	u.u_error = copyout(&result, uap->result, sizeof(int));
}  

void
new_ipcrecv()
{
	struct a {
		int	vcdesc;
		caddr_t	data;
		caddr_t	dlen;
		caddr_t	flags;
		caddr_t	opt;
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	caddr_t	dlen_addr;
	int	dlen;
	int	result=0;

	dlen_addr = uap->dlen;
	if (copyin(uap->dlen, (caddr_t)&dlen, sizeof(dlen)))
		result = NSR_BOUNDS_VIO;

	if (!result) {
		/* the ipcrecv syscall expects a dlen VALUE
		/* for historical reasons
		*/
		uap->dlen = (caddr_t) dlen;
		result = ipcrecv();
		if (result == 0)
			if (copyout(&u.u_rval1, dlen_addr, sizeof(int)))
				result = NSR_BOUNDS_VIO;
	}

	u.u_error = copyout(&result, uap->result, sizeof(int));
}

 
void
new_ipcrecvcn()
{
	struct a {
		int	calldesc;
		caddr_t	vcdesc;
		caddr_t	flags;
		caddr_t	opt;
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipcrecvcn();
	u.u_error = copyout(&result, uap->result, sizeof(int));
} 
void
new_ipcselect()
{
	struct a {
		int	*sdbound;
		fd_set	*rmap;
		fd_set	*wmap;
		fd_set	*emap;
		int	timeout;
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipcselect();
	u.u_error = copyout(&result, uap->result, sizeof(int));
}

 
void
new_ipcsend()
{
	struct a {
		int	vcdesc;
		caddr_t data;
		int	dlen;
		caddr_t flags;
		caddr_t	opt;
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipcsend();
	u.u_error = copyout(&result, uap->result, sizeof(int));
}


void
new_ipcsetnodename()

{
	/* set up pointers to caller's parameters */
	 struct a {
		caddr_t nodename;	/* nodename to be assigned */
		int	nodelen;	/* byte length of nodename */
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipcsetnodename();
	u.u_error = copyout(&result, uap->result, sizeof(int));
}  /* setnodename */

 
void
new_ipcshutdown()
{
	struct a {
		int	descriptor;
		caddr_t	flags;
		caddr_t	opt;
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int result;
	result = ipcshutdown();
	u.u_error = copyout(&result, uap->result, sizeof(int));
}
