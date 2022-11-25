/*
 * $Header: nipc_compat.c,v 1.3.83.5 93/12/09 13:34:24 marshall Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_compat.c,v $
 * $Revision: 1.3.83.5 $		$Author: marshall $
 * $State: Exp $		$Locker:  $
 * $Date: 93/12/09 13:34:24 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nipc_compat.c $Revision: 1.3.83.5 $";
#endif

/* 
 * Netipc compatible initialization code.
 */


#include "../h/types.h"
#include "../h/param.h"
#include "../h/file.h"
#include "../h/ioctl.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/signal.h"
#include "../h/syscall.h"
#include "../h/systm.h"
#include "../h/ns_ipc.h"



nipc_compat_init()
{
	int	 compat_ipcconnect();
	int	 compat_ipccontrol();
	int	 compat_ipccreate();
	int	 compat_ipcdest();
	int	 compat_ipclookup();
	int	 compat_ipcname();
	int	 compat_ipcnamerase();
	int	 compat_ipcrecv();
	int	 compat_ipcrecvcn();
	int	 compat_ipcselect();
	int	 compat_ipcsend();
	int	 compat_ipcshutdown();

	/* initialize the old syscall entry points */
	sysent_assign(209,	6,	 compat_ipcconnect);
	sysent_assign(215,	8,	 compat_ipccontrol);
	sysent_assign(203,	6,	 compat_ipccreate);
	sysent_assign(221,	10,	 compat_ipcdest);
	sysent_assign(206,	9,	 compat_ipclookup);
	sysent_assign(204,	4,	 compat_ipcname);
	sysent_assign(205,	3,	 compat_ipcnamerase);
	sysent_assign(212,	6,	 compat_ipcrecv);
	sysent_assign(210,	5,	 compat_ipcrecvcn);
	sysent_assign(211,	6,	 compat_ipcsend);
	sysent_assign(220,	4,	 compat_ipcshutdown);

#ifdef	__hp9000s300
	/* initialize the backward compatibility table */
	/* see netioctl.c for more info */
	{
	struct map_table {
		int num_args;
		int (*handler)();
	};

	extern struct map_table map_table[];

#define maptable_assign(a,b,c) { \
	map_table[a].num_args = b; \
	map_table[a].handler = c; \
}

		maptable_assign(26, 	6,	compat_ipcconnect);
		maptable_assign(27,	8,	compat_ipccontrol);
		maptable_assign(28,	6,	compat_ipccreate);
		maptable_assign(29,	10,	compat_ipcdest);
		maptable_assign(30,	6,	compat_ipcrecv);
		maptable_assign(31,	5,	compat_ipcrecvcn);
		maptable_assign(32,	6,	compat_ipcselect);
		maptable_assign(33,	6,	compat_ipcsend);
		maptable_assign(34,	4,	compat_ipcshutdown);
		/* #35 is unused by netipc */
		maptable_assign(36,	4,	compat_ipcname);
		maptable_assign(37,	3,	compat_ipcnamerase);
		maptable_assign(38,	9,	compat_ipclookup);
	}
#endif
}

compat_ipcconnect()
{
	 struct a {
		int	calldesc;	/* CALL SOCKET IS IGNORED */
		int	destdesc;	/* POINTER TO A 'DEST DESC' */
		caddr_t flags;
		caddr_t opt;
		caddr_t vcdesc;
		caddr_t	result;
	} *uap = (struct a *)u.u_ap;

	int	result;
	result = ipcconnect();
#ifdef	__hp9000s300
	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;
#else
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
} 
 
compat_ipccontrol() 
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

/* old versions of code may try to use undocumented requests which are no
 * longer supported. Trap those now illegal requests here.
 */
#define NSC_SET_NODE_NAME	9007
#define NSC_TRACING_ENABLE	259
#define NSC_ISTHISME		9003
#define NSC_QUALIFY_NODE_NAME	9009
#define NSC_SETEUID		9010
	
	if (uap->request == NSC_SET_NODE_NAME ||
	    uap->request == NSC_TRACING_ENABLE ||
	    uap->request == NSC_ISTHISME ||
	    uap->request == NSC_QUALIFY_NODE_NAME ||
	    uap->request == NSC_SETEUID)
		result = NSR_REQUEST;
	else
		result = ipccontrol();

#ifdef	__hp9000s300
	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;
#else
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
}

 

compat_ipccreate()
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
#ifdef	__hp9000s300
	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;
#else
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
} 
		
 


compat_ipcdest()
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
#ifdef	__hp9000s300
	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;
#else
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
}  



compat_ipclookup()
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
#ifdef	__hp9000s300
	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;
#else
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
}  




compat_ipcname()
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
#ifdef	__hp9000s300
	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;
#else
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
}  



compat_ipcnamerase()
{
	/* set up pointers to caller's parameters */
	 struct a {
		caddr_t socketname;	/* name to bind to descriptor  */
		int	nlen;		/* byte length of socketname   */
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipcnamerase();
#ifdef	__hp9000s300
	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;
#else
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
}  


compat_ipcrecv()
{
#ifdef	__hp9000s300
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
	}

	if (result == 0)
		if (copyout(&u.u_rval1, dlen_addr, sizeof(int)))
			u.u_error = NSR_BOUNDS_VIO;

	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;

#else	/* series 800 backward compatible */
	struct a {
		int	vcdesc;
		caddr_t	data;
		int	dlen;
		caddr_t	flags;
		caddr_t	opt;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipcrecv();
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
}

 

compat_ipcrecvcn()
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
#ifdef	__hp9000s300
	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;
#else
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
} 

compat_ipcselect()
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
#ifdef	__hp9000s300
	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;
#else
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
}

 

compat_ipcsend()
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
#ifdef	__hp9000s300
	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;
#else
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
}
 

compat_ipcshutdown()
{
	struct a {
		int	descriptor;
		caddr_t	flags;
		caddr_t	opt;
		caddr_t	result;
	} *uap = (struct a *) u.u_ap;

	int	result;
	result = ipcshutdown();
#ifdef	__hp9000s300
	u.u_error = copyout(&result, uap->result, sizeof(int));
	u.u_rval1 = 0;
#else
	if (result)
		u.u_error = result + NIPC_ERROR_OFFSET;
	else
		u.u_error = 0;
#endif
}
