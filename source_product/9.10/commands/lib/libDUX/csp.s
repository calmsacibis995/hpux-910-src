/* @(#) $Revision: 64.1 $
 *
 * csp.s -- kernel entry point for csp()
 *
 */
#ifdef __hp9000s800
#include <sys/syscall.h>
#endif

#ifndef CSP
#   define CSP 168
#endif

#ifdef __hp9000s300
	global	_csp
	global	__cerror

_csp:
	mov.l	&CSP,%d0
	trap	&0
	bcc.b	noerror
	jmp	__cerror
noerror:
	rts
#endif

#ifdef __hp9000s800

#ifndef CN
#   define CN      r22              /* System call number (New Call) */
#   define RS      r22              /* Return status (New Call) */
#   define SYSCALLGATE  0xc0000004  /* Offsets of gate instructions */
#endif
	.space	$TEXT$
	.subspa	$CODE$
	.export	csp,entry
	.proc
	.callinfo
	.entry
csp
	.call
	ldil	L%SYSCALLGATE,r1
	ble	R%SYSCALLGATE(sr7,r1)
	ldi	CSP,CN
	or,=	r0,RS,r0
	.call
	.import	$cerror
	b,n	$cerror
	nop
	bv,n	(rp)
	nop
	.procend
	.end
#endif
