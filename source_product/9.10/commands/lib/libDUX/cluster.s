/* @(#) $Revision: 64.1 $
 *
 * cluster.s -- hidden kernel entry point for cluster()
 *
 * error = cluster(multiaddr, maxnode, type, site_id, site_name, knsp);
 */
#ifdef __hp9000s800
#    include <sys/syscall.h>
#endif

#ifndef CLUSTER
#   define CLUSTER 169
#endif

#ifdef __hp9000s300
	global	_cluster
	global	__cerror

_cluster:
	mov.l	&CLUSTER,%d0
	trap	&0
	bcc.w	noerror
	jmp	__cerror
noerror:
	clr.l	%d0
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
	.export	cluster,entry
	.proc
	.callinfo
	.entry
cluster
	.call
	ldil	L%SYSCALLGATE,r1
	ble	R%SYSCALLGATE(sr7,r1)
	ldi	CLUSTER,CN
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
