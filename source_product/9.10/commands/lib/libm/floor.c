/* @(#) $Revision: 64.5 $ */      
/*  source:  System III                                    */
/*     /usr/src/lib/libm/floor.c                           */
/*     dsb - 3/8/82                                        */

/*
 * floor and ceil-- greatest integer <= arg
 * (resp least >=)
 */

#ifdef hp9000s200

#ifdef _NAMESPACE_CLEAN
#define _afadd     ___afadd
#define _ceil     __ceil
#define _floor     __floor
#define _modf     __modf
#endif /* _NAMESPACE_CLEAN */

#ifdef _NAMESPACE_CLEAN
#undef _floor
#pragma _HP_SECONDARY_DEF _floor floor
#define floor _floor
#endif /* _NAMESPACE_CLEAN */

#pragma OPT_LEVEL 1

double floor(d) double d;

#ifdef _NAMESPACE_CLEAN
#undef floor
#define _floor __floor
#endif /* _NAMESPACE_CLEAN */

{
	asm("	tst.w	flag_68881
		bne.w	floor881
		bclr	&7,(8,%a6)
		beq.b	floor3
		pea	(8,%a6)
		move.l	(12,%a6),-(%sp)
		move.l	(8,%a6),-(%sp)
		jsr	_modf
		lea	(12,%sp),%sp
		tst.l	%d1
		bne.b	floor1
		move.l	%d0,%d1
		neg.l	%d0
		cmp.l	%d1,%d0
		beq.b	floor2");
	asm("floor1:
		clr.l	-(%sp)
		move.l	&0x3FF00000,-(%sp)
		pea	(8,%a6)
		jsr	_afadd
		lea	(12,%sp),%sp");
	asm("floor2:
		bchg	&7,(8,%a6)
		bra.b	floor4");
	asm("floor3:
		pea	(8,%a6)
		move.l (12,%a6),-(%sp)
		move.l	(8,%a6),-(%sp)
		jsr	_modf
		lea	(12,%sp),%sp");
	asm("floor4:
		move.l	(12,%a6),%d1
		move.l	(8,%a6),%d0
		bra.b	floor_1");

	asm("floor881:
		fmov.l	%fpcr,%d0		#save control register
		movq	&0x20,%d1
		fmov.l	%d1,%fpcr
		fint.d	8(%a6),%fp0		#fp0 <- int(arg)
		fmov.d	%fp0,-(%sp)		#save result
		fmov.l	%d0,%fpcr		#restore control register
		mov.l	(%sp)+,%d0
		mov.l	(%sp)+,%d1");
	asm("floor_1:");

}

#ifdef _NAMESPACE_CLEAN
#undef _ceil
#pragma _HP_SECONDARY_DEF _ceil ceil
#define ceil _ceil
#endif /* _NAMESPACE_CLEAN */

double ceil(d) double d;

#ifdef _NAMESPACE_CLEAN
#undef ceil
#define _ceil __ceil
#endif /* _NAMESPACE_CLEAN */

{
	asm("	tst.w	flag_68881
		bne.w	ceil881
		move.l	(12,%a6),-(%sp)
		move.l	(8,%a6),-(%sp)
		bchg	&7,(%sp)
		jsr	_floor
		addq	&8,%sp
		bchg	&31,%d0
		bra.b	ceil_1");

	asm("ceil881:
		fmov.l	%fpcr,%d0		#save control register
		movq	&0x30,%d1
		fmov.l	%d1,%fpcr
		fint.d	8(%a6),%fp0		#fp0 <- int(arg)
		fmov.d	%fp0,-(%sp)		#save result
		fmov.l	%d0,%fpcr		#restore control register
		mov.l	(%sp)+,%d0
		mov.l	(%sp)+,%d1");
	asm("ceil_1:");

}

#pragma OPT_LEVEL 2

#else /* hp9000s200 */

#ifdef _NAMESPACE_CLEAN
#define floor _floor
#define ceil _ceil
#define modf _modf
#endif /* _NAMESPACE_CLEAN */

double	modf();

#ifdef _NAMESPACE_CLEAN
#undef floor
#pragma _HP_SECONDARY_DEF _floor floor
#define floor _floor
#endif /* _NAMESPACE_CLEAN */

double floor(d) double d;
{
	double fract;

	if (d<0.0) {
		d = -d;
		fract = modf(d, &d);
		if (fract != 0.0)
			d += 1;
		d = -d;
	} else
		modf(d, &d);
	return(d);
}

#ifdef _NAMESPACE_CLEAN
#undef ceil
#pragma _HP_SECONDARY_DEF _ceil ceil
#define ceil _ceil
#endif /* _NAMESPACE_CLEAN */

double ceil(d) double d;
{
	return(-floor(-d));
}

#endif /* hp9000s200 */
