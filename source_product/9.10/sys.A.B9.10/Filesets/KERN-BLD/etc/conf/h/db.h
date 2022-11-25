/*
 * @(#)db.h: $Revision: 1.4.83.4 $ $Date: 93/09/17 18:24:30 $
 * $Locker:  $
 */

#ifdef _KERNEL
struct	dbipc {
	int ipc_flags;		/* flags */
	int ipc_message;	/* message */
	struct proc *ipc_procp;	/* other process */
};

#define DBIPC_BLOCKED		0x01	/* blocked, waiting */
#define DBIPC_GONE		0x02	/* other process called db_close(), exit() or exec() */ 
#define	DBIPC_EVENT_POSTED	0x04	/* message present */
#define	DBIPC_SETRQ		0x08	/* process has called setrq() */
#define	DBIPC_OPEN_PENDING	0x10	/* open pending */
#define	DBIPC_CLOSE_PENDING	0x20	/* close pending */

#endif
