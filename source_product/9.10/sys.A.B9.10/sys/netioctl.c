/* $Header: netioctl.c,v 1.3.84.3 93/09/17 21:20:56 kcs Exp $ */

/* 
 * Netioctl.c
 *
 * The biggest hack in all of networking.  This module is provided
 * for backward compatability only!!  NO FUTURE ENHANCEMENTS TO NETWORKING
 * SHOULD EVER BE REFLECTED IN THIS MODULE!!
 * 
 * The story goes like this.  Back in the LNA days, all networking 
 * system calls entered the kernel through a single entry point, netioctl.
 * To accomplish this, the library stubs would take the parameters off the
 * stack, malloc a structure large enough to hold them all, copy them
 * into this structure, push this address onto the stack, and trap into
 * the kernel.  So when we get here we have a system call id (which,
 * incidently, is not the same as the sysent number) and a pointer to
 * a block of memory containing the actual arguments to the system call.
 *
 * The reasons for this are lost in obscurity, but there is no reason to
 * do it this way today.  NEW DEVELOPMENT SHOULD TRAP DIRECTLY INTO THE
 * KERNEL!!  All of the 8.0 (and subsequent) libraries have stubs
 * which reflect these new semantics.  The only thing that will ever
 * get to this module is programs which have not been recompiled
 * under 8.0 (or later) libraries (i.e. this module is an object code
 * compatability hack).  
 *
 * The idea is simple.  Figure out how many parameters a given system
 * call is supposed to have, copyin that structure, move that number
 * of things (all treated as ints) to u_arg, and call the
 * appropriate thing. When it returns, we return, as no additional
 * work need be done.  Note that things have been intentionally defined
 * here, so they never change.
 */

#include "../h/param.h"
#include "../h/types.h"
#include "../h/user.h"

extern int compat_socket();
extern int compat_listen();
extern int compat_bind();
extern int compat_accept();
extern int compat_connect();
extern int compat_recv();
extern int compat_send();
extern int compat_shutdown();
extern int compat_getsockname();
extern int compat_setsockopt();
extern int compat_sendto();
extern int compat_recvfrom();
extern int compat_getpeername();
extern int compat_getsockopt();
extern int nosys();

struct map_table {
	int num_args;
	int (*handler)();
} map_table[] = {
	{ 0,	nosys,			/*   0 = RFA_NETUNAM_ID */	},
	{ 0,	nosys,			/*   1 = RFA_SRV_FCHDIR_ID */	},
	{ 0,	nosys,			/*   2 = RFA_SRV_..._ID */	},
	{ 0,	nosys,			/*   3 = RFA_SRV_..._ID */	},
	{ 0,	nosys,			/*   4 = RFA_SRV_..._ID */	},
	{ 0,	nosys,			/*   5 = RFA_SRV_SETREMOTE_ID */},
	{ 3,	compat_socket,		/*   6 = SOCKET_ID */		},
	{ 2,	compat_listen,		/*   7 = LISTEN_ID */		},
	{ 3,	compat_bind,		/*   8 = BIND_ID */		},
	{ 3,	compat_accept,		/*   9 = ACCEPT_ID */		},
	{ 3,	compat_connect,		/*  10 = CONNECT_ID */		},
	{ 4,	compat_recv,		/*  11 = SRECV_ID */		},
	{ 4,	compat_send,		/*  12 = SSEND_ID */		},
	{ 2,	compat_shutdown,	/*  13 = SHUTDOWN_ID */		},
	{ 3,	compat_getsockname,	/*  14 = GETSOCKNAME_ID */	},
	{ 5,	compat_setsockopt,	/*  15 = SETSOCKOPT_ID */	},
	{ 6,	compat_sendto,		/*  16 = SENDTO_ID */		},
	{ 6,	compat_recvfrom,	/*  17 = RECVFROM_ID */		},
	{ 3,	compat_getpeername,	/*  18 = GETPEERNAME_ID */	},
	{ 0,	nosys,			/*  19 = BFA_COPY_ID */		},
	{ 0,	nosys,			/*  20 = BFA_ZERO_ID */		},
	{ 0,	nosys,			/*  21 = KSET_TRIGGER_ID */	},
	{ 0,	nosys,			/*  22 = KCLEAR_TRIGGER_ID */	},
	{ 0,	nosys,			/*  23 = KTRY_TRIGGER_ID */	},
	{ 0,	nosys,			/*  24 = KTRIGGER_DUMP_ID */	},
	{ 0,	nosys,			/*  25 = KUSET_ID */		},
	{ 0,	nosys,			/*  26 = IPCCONNECT_ID */	},
	{ 0,	nosys,			/*  27 = IPCCONTROL_ID */	},
	{ 0,	nosys,			/*  28 = IPCCREATE_ID */	},
	{ 0,	nosys,			/*  29 = IPCDEST_ID */		},
	{ 0,	nosys,			/*  30 = IPCRECV_ID */		},
	{ 0, 	nosys,			/*  31 = IPCRECVCN_ID */	},
	{ 0,	nosys,			/*  32 = IPCSELECT_ID */	},
	{ 0,	nosys,			/*  33 = IPCSEND_ID */		},
	{ 0,	nosys,			/*  34 = IPCSHUTDOWN_ID */	},
	{ 5,	compat_getsockopt,	/*  35 = GETSOCKOPT_ID */	},
	{ 0,	nosys,			/*  36 = IPCNAME_ID */		},
	{ 0,	nosys,			/*  37 = IPCNAMERASE_ID */	},
	{ 0,	nosys,			/*  38 = IPCLOOKUP_ID */	},
	{ 0,	nosys,			/*  39 = IPCRECVREQ_ID */	},
	{ 0,	nosys,			/*  40 = IPCSENDREPLY_ID */	},
	{ 0,	nosys,			/*  41 = IPCSENDREQ_ID  */	},
	{ 0,	nosys,			/*  42 = IPCRECVREPLY_ID */	},
	{ 0,	nosys,			/*  43 = IPCSENDTO_ID */	},
	{ 0,	nosys,			/*  44 = IPCRECVFROM_ID */	},
	{ 0,	nosys,			/*  45 = IPCMUXCONNECT_ID */	},
	{ 0,	nosys,			/*  46 = IPCMUXRECV_ID */	},
	{ 0,	nosys,			/*  47 = IPCGIVE_ID */		},
	{ 0,	nosys,			/*  48 = IPCGETSOCK_ID */	},

};

#define MAP_TABLE_SIZE	(sizeof(map_table) / sizeof(map_table[0]))

#define KERN_ENV_ROUT			1000	/* from h/nusinf.h */
#define	MAX_PARMS			10	/* max used in syscalls */
	
void
netioctl()
{
	register struct a {
		int	syscall_id;
		caddr_t	parm_blk;
	} *uap = (struct a *)u.u_ap;

	int parms[MAX_PARMS];
	int map_index, i;

	map_index = uap->syscall_id - KERN_ENV_ROUT;

	if (map_index < 0 || map_index > MAP_TABLE_SIZE) {
		printf("netioctl: unknown syscall: %d\n", uap->syscall_id);
		nosys();  /* will gen signal and put right stuff in u.u_error */
		return;
	}

	if (map_table[map_index].num_args) {
		if (u.u_error = copyin(uap->parm_blk, parms, 
		    map_table[map_index].num_args*sizeof(int)))
			return;
	
		for (i=0; i < map_table[map_index].num_args; i++)
			u.u_arg[i] = parms[i];
		u.u_ap = u.u_arg;
	}

	(*map_table[map_index].handler)(); /* lets get some real work done */
		
}
