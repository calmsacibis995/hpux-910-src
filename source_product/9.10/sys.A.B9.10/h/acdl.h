/*
 * @(#)acdl.h: $Revision: 1.2.83.4 $ $Date: 93/11/22 09:57:42 $
 * $Locker:  $
 */

/* defines for the flags */

#define DEADLOCKED   0x1
#define RECOVERED    0x2
#define DL_TIMER_ON  0x4
#define DEADLOCK_TIMEOUT 20

struct ac_deadlock_struct {
	int flags;
	int (*recover_func)();
	caddr_t dl_data;
};
