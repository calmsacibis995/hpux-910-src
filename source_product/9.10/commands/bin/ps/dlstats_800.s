#include <sys/syscall.h>

#ifndef SYS_DSKLESS_STATS
#  define SYS_DSKLESS_STATS 184
#endif

	.code
	.proc
	.callinfo
	.entry
	.export dskless_stats,entry
dskless_stats
	ldil    L%SYSCALLGATE,r1
	ble     R%SYSCALLGATE(sr7,r1)
	ldi     SYS_DSKLESS_STATS,r22
	or,=    r0,r22,r0
	.import $cerror
	b,n     $cerror
	nop
	bv,n    r0(rp)
	nop
	.procend
