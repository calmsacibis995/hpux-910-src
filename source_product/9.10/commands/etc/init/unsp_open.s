/* @(#) $Revision: 64.1 $
 *
 * unsp_open.s -- hidden kernel entry point for unsp_open()
 *
 */
#ifdef __hp9000s800
#    include <sys/syscall.h>
#endif

#ifndef UNSP_OPEN
#   define UNSP_OPEN 172
#endif

#ifdef __hp9000s300
	global	_unsp_open
	global	__cerror

_unsp_open:
	mov.l	&UNSP_OPEN,%d0
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
	.export	unsp_open,entry
	.proc
	.callinfo
	.entry
unsp_open

	.call
	ldil	L%SYSCALLGATE,r1
	ble	R%SYSCALLGATE(sr7,r1)
	ldi	UNSP_OPEN,CN
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
