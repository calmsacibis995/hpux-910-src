#include <sys/types.h>
#include <sys/param.h>
#ifdef hp9000s800
#include <nlist.h>  /* INCLUDED INDIRECTLY BY sys/user.h ON SERIES 300 */
#endif hp9000s800
#include <stdio.h>
#include <mntent.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/unistd.h>
#ifdef CURSES
#include <curses.h>
#endif /* CURSES */
#include <termio.h>
#include <sys/ioctl.h>
#include <netio.h>
#include <pwd.h>
#include <sys/dk.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/stat.h>
#include <sys/fs.h>
#ifdef SWFS
#include <sys/vfs.h>
#endif SWFS
#include <sys/lock.h>
#include <sys/utsname.h>
#include <sys/rtprio.h>
#include <sys/sysmacros.h>
#include <sys/map.h>
#include <sys/conf.h>

#include <sys/file.h>
#include <sys/vnode.h>
#ifdef hp9000s200
#include <machine/bootrom.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif hp9000s200

#include <sys/nsp.h>
#define ADDRESS_SIZE 6
#include <sys/cct.h>
#include <sys/swap.h> 
  /* #include <sys/rmswap.h> */
#include <cluster.h>
#include <sys/protocol.h>
#include <sys/dux_mbuf.h>
#include <sys/pstat.h>

#include "rwhod.h"	/* Copy of the one that rwhod.c compiles with */

#ifdef hp9000s800
/*
 * pindx is precomputed and stored in the proc struct, but we don't
 * want to use the pindx from the proc struct (we would have to read it
 * from /dev/kmem).  Therefore we use the old macro.  The precomputation
 * is done to save division calls in the kernel.  We would rather do a
 * divison instead of a read.
 */
#undef pindx
#define pindx(p) ((p) - proc)
#endif hp9000s800


#ifdef MAIN
#define declare
#else
#define declare extern
#endif

typedef int boolean;
#define true 1
#define false 0

extern char whatstring[];
#ifdef lint
#define BUILT_ON	"lint"
#endif
#define buildstring  BUILT_ON

declare boolean osdesignermode;
declare	boolean in_screen;

#define NICEVAL -10
#define ROWS 24
#define COLUMNS 80
#define MAX_ESCAPE 128

#define CR 13
#define LF 10
#define QUOTE 39
#define TAB 9
declare char hardcopy[80];
#define NREVISION "1.0"

#ifdef MAIN
int update_interval = 1;
#else
extern int update_interval;
#endif

#define max(a,b) ((a) > (b) ? (a) : (b))

struct modes {
	char letter;
	int (*routine)(), (*init)();
	char *label;
	int screen_number;
};
