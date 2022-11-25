#include "sys/syscall.h"

#define	SYSCALL(x)\
ENTRY(x);\
	DOTHECALL(x)

#define	PSEUDO(x,y)\
ENTRY(x);\
	DOTHECALL(y)

#ifndef	UNDERSCORE

/* _mcount(frompcindex, selfpc, cntp) */
#define PROFCALL(x) \
	ldo	48(sp),sp;\
	copy	rp,arg0;\
	ldil	L%/**/x,arg1;\
	ldo	R%/**/x(arg1),arg1;\
	ldil	L%cnt,arg2;\
	ldo	R%cnt(arg2),arg2;\
	.import	_mcount;\
	ldil	L%_mcount,r31;\
	.call;\
	ble	R%_mcount,(4,r31);\
	copy	r31,rp;\
	ldo	-48(sp),sp;\
	ldw	-20(sp),rp;\
	ldw	-36(sp),arg0;\
	ldw	-40(sp),arg1;\
	ldw	-44(sp),arg2;\
	ldw	-48(sp),arg3

#ifdef PROF
#define	ENTRY(x)\
	.space	$PRIVATE$;\
	.subspa	$DATA$;cnt;\
	.word	0;\
	.space	$TEXT$;\
	.subspa	$CODE$;\
	.export	/**/x,entry;\
	.proc;\
	.callinfo save_rp;\
	.entry;\
_/**/x;\
	stw	rp,-20(sp);\
	stw	arg0,-36(sp);\
	stw	arg1,-40(sp);\
	stw	arg2,-44(sp);\
	stw	arg3,-48(sp);\
	PROFCALL(x)
#else not PROF
#define	ENTRY(x)\
	.space	$TEXT$;\
	.subspa	$CODE$;\
	.export	/**/x,entry;\
	.proc;\
	.callinfo;\
	.entry;\
_/**/x;
#endif	not PROF

#define	DOTHECALL(x)\
.call;\
	ldil	L%SYSCALLGATE,r1;\
	ble	R%SYSCALLGATE(sr7,r1);\
	ldi	SYS_/**/x,SYS_CN;\
	or,=	r0,RS,r0;\
	.call;\
	.import	$cerror;\
	b,n	$cerror;\
	nop

#define	RET \
bv,n	(rp);\
	nop;\
	.procend;\
	.end

#define	IENTRY(x)\
	.space	$TEXT$;\
	.subspa	$CODE$;\
	.export	/**/x,entry;\
	.proc;\
	.callinfo hpux_int, frame=472;\
	.entry;\
_/**/x;

#else	UNDERSCORE

/* _mcount(frompcindex, selfpc, cntp) */
#define PROFCALL(x) \
	ldo	48(sp),sp;\
	copy	rp,arg0;\
	ldil	L%_/**/x,arg1;\
	ldo	R%_/**/x(arg1),arg1;\
	ldil	L%cnt,arg2;\
	ldo	R%cnt(arg2),arg2;\
	.import	__mcount;\
	ldil	L%__mcount,r31;\
	.call;\
	ble	R%__mcount,(4,r31);\
	copy	r31,rp;\
	ldo	-48(sp),sp;\
	ldw	-20(sp),rp;\
	ldw	-36(sp),arg0;\
	ldw	-40(sp),arg1;\
	ldw	-44(sp),arg2;\
	ldw	-48(sp),arg3

#ifdef PROF
#define	ENTRY(x)\
	.space	$PRIVATE$;\
	.subspa	$DATA$;cnt;\
	.word	0;\
	.space	$TEXT$;\
	.subspa	$CODE$;\
	.export	_/**/x,entry;\
	.proc;\
	.callinfo save_rp;\
	.entry;\
__/**/x;\
	stw	rp,-20(sp);\
	stw	arg0,-36(sp);\
	stw	arg1,-40(sp);\
	stw	arg2,-44(sp);\
	stw	arg3,-48(sp);\
	PROFCALL(x)
#else not PROF
#define	ENTRY(x)\
	.space	$TEXT$;\
	.subspa	$CODE$;\
	.export	_/**/x,entry;\
	.proc;\
	.callinfo;\
	.entry;\
__/**/x;
#endif	not PROF

#define	DOTHECALL(x)\
.call;\
	ldil	L%SYSCALLGATE,r1;\
	ble	R%SYSCALLGATE(sr7,r1);\
	ldi	SYS_/**/x,SYS_CN;\
	or,=	r0,RS,r0;\
	.call;\
	.import	cerror;\
	b,n	cerror;\
	nop

#define	RET \
bv,n	(rp);\
	nop;\
	.procend;\
	.end

#define	IENTRY(x)\
	.space	$TEXT$;\
	.subspa	$CODE$;\
	.export	_/**/x,entry;\
	.proc;\
	.callinfo hpux_int, frame=472;\
	.entry;\
__/**/x;

#endif	UNDERSCORE
