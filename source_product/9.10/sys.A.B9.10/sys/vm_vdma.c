/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_vdma.c,v $
 * $Revision: 1.2.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/18 13:46:50 $
 */

/*
 * Hardware independent portions of VDMA support.
 *
 * Notification -
 *   int  vdma_install()
 *   int  vdma_uninstall()
 *   int  vdma_notify()
 */

#include "../h/types.h"
#include "../h/vas.h"
#include "../h/vdma.h"

#ifndef NULL
#define NULL 0
#endif


/*
 * This routine is called by the VM routines to notfify any
 * and all VDMA devices of a change to a translation or
 * set of them.  The vnotify structure fields contain the
 * various specifics about the translation change.
 */

int (*vdma_clients[_MAX_VDMA_DEVICES])() = {
	NULL
};
unsigned char vdma_client_mask = 0;	/* 
					 * NB this caps _MAX_VDMA_DEVICES at 8.
					 */

vdma_install(fptr)
	register int (*fptr)();
{
	register int i;
	if (vdma_client_mask == 0xff) {
		return(0);	/* table full */
	}
	for (i = 0; i < _MAX_VDMA_DEVICES; i++) {
		if (vdma_clients[i] == NULL) {
			vdma_clients[i] = fptr;
			vdma_client_mask |= (1 << i);
			return(1);
		}
	}
	/* shouldn't get here (unless MP?) */
	return(0);	/* no free slots */
}


#ifdef NEVER_CALLED
vdma_uninstall(fptr)
	register int (*fptr)();
{
	register int i;
	for (i = 0; i < _MAX_VDMA_DEVICES; i++) {
		if (vdma_clients[i] == fptr) {
			vdma_clients[i] = NULL;
			vdma_client_mask &= ~(1 << i);
			return(1);
		}
	}
	return(0);	/* did not have an entry */
}
#endif /* NEVER_CALLED */

vdma_notify(vnp)
	register struct vnotify *vnp;
{
	register int (*fptr)();
	register int i;
	register unsigned char mask = vdma_client_mask;
	register int retval = 0;
       
	for (i = 0; i < _MAX_VDMA_DEVICES; i++) {
		if (!mask) { 		/* no more clients to send to */
			return(retval);	/* so shortcut the loop */
		}
		if ((fptr = vdma_clients[i])) {
			retval |= (*fptr)(vnp); /* call the client */
			mask &= ~(1 << i);
		}
	}
	return(retval);
}

