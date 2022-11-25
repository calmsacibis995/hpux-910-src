/*

    Dprint_stat_code()

    Print code to calculate and print average time.

    This is a generic routine that is printed for performance analysis.
    The only variable in the entire code segment is the number of
    iterations which is specified by means of two "set" pseudo-ops (the
    upper and lower halves of the double precision representation
    called IHIGH and ILOW). The code simply does the calculation:

		    total time - overhead time
	avg. time = --------------------------
		      number of iterations

    The bulk of the code, however, is involved in getting the average
    time into the proper format to be printed. This code also does the
    printing itself.

	set	IHIGH,{iterations_high}
	set	ILOW,{iterations_low}
	data
	lalign	4
stat1:	space	8			# total time end
stat2:	space	8			# total time start
stat3:	space	8			# overhead time end
stat4:	space	8			# overhead time start
stat5:	space	8			# timezone dummy argument
stat6:	space	28			# print buffer
stat7:	byte	"avg. time:",9,0	# print field name
stat8:	byte	"0.0 sec",10,10,0	# for <1.0 nanosecond
stat9:	byte	"%4d hr "		# stat9, stat10, & stat11 
stat10:	byte	"%2d min "		#      must be consecutive
stat11:	byte	"%6.3f sec",10,10,0
stat12:	byte	"%7.3f *sec",10,10,0	# for "nsec", "usec", and "msec"
stat13:	byte	"%s",0			# for printing print buffer
stat14:	byte	"time overflow",10,10,0	# for >= 10000 hours
	text
	pea	stat7
	mov.l	file,-(%sp)
	jsr	_fprintf		# print field name
	addq.w	&8,%sp
	lea	stat1,%a0		# pointer to time values
	mov.l	&1000000,%d0		# microseconds per second
	movm.l	(%a0)+,%d2-%d5		# get total time values
	sub.l	%d5,%d3			# subtract microseconds
	bcc.b	stat15			# need to borrow a second?
	add.l	%d0,%d3			# yes: adjust microseconds
	subq.l	&1,%d2			# one less second
stat15:	sub.l	%d4,%d2			# subtract secs (d2-d3 is total time)
	movm.l	(%a0),%d4-%d7		# get overhead time values
	sub.l	%d7,%d5			# subtract microseconds
	bcc.b	stat16			# need to borrow a second?
	add.l	%d0,%d5			# yes: adjust microseconds
	subq.l	&1,%d4			# one less second
stat16:	sub.l	%d6,%d4			# subtract secs (d4-d5 is ovhd time)
	sub.l	%d5,%d3			# subtract ovhd from total time
	bcc.b	stat17			# need to borrow?
	add.l	%d0,%d3			# yes: adjust microseconds
	subq.l	&1,%d2			# one less second
stat17:	sub.l	%d4,%d2			# subtract secs (d2-d3 is result time)
        bcs.b	stat18			# in case of quantization problem
	mov.l	%d3,-(%sp)		# convert usecs to double
	jsr	_floatu
	mov.l	%d1,(%sp)
	mov.l	%d0,-(%sp)
	mov.l	&0xA0B5ED8D,-(%sp)	# 1E-6
	mov.l	&0x3EB0C6F7,-(%sp)
	jsr	_fmul			# usecs*1E-6
	addq.w	&8,%sp
	movm.l	%d0-%d1,(%sp)
	mov.l	%d2,-(%sp)
	jsr	_floatu			# convert secs to double
	mov.l	%d1,(%sp)
	mov.l	%d0,-(%sp)
	jsr	_fadd			# total number of seconds
	mov.l	&ILOW,%d5		# double precision representation
	mov.l	&IHIGH,%d4		#     of the number of iterations
	movm.l	%d0-%d1/%d4-%d5,(%sp)
	jsr	_fdiv
	lea	16(%sp),%sp		# d0-d1 is average time
	cmp.l	%d0,&0x3E112E0B		# compare with 1E-9
	bgt.b	stat19
	bne.b	stat18
	cmp.l	%d1,&0xE826D695
	bcc.b	stat19
stat18:	pea	stat8			# < 1 nsec; print "0.0 sec"
	mov.l	file,-(%sp)
	jsr	_fprintf
	addq.w	&8,%sp
	bra.w	stat37
stat19:	cmp.l	%d0,&0x3EB0C6F7		# compare with 1E-6
	bhi.b	stat21
	bne.b	stat20
	cmp.l	%d1,&0xA0B5ED8D
	bcc.b	stat21
stat20:	mov.l	&0x41CDCD65,%d2		# upper four bytes of 1E9
	movq	&'n,%d4			# "n" for "nsec"
	bra.b	stat24
stat21:	cmp.l	%d0,&0x3F50624D		# compare with 1E-3
	bhi.b	stat23
	bne.b	stat22
	cmp.l	%d1,&0xD2F1A9FC
	bcc.b	stat23
stat22:	mov.l	&0x412E8480,%d2		# upper four bytes of 1E6
	movq	&'u,%d4			# "u" for "usec"
	bra.b	stat24
stat23:	cmp.l	%d0,&0x3FF00000		# compare with 1.0
	bcc.b	stat25
	mov.l	&0x408F4000,%d2		# upper four bytes of 1E3
	movq	&'m,%d4			# "m" for "msec"
stat24:	movq	&0,%d3
	movm.l	%d0-%d3,-(%sp)		# convert nsec, usec, or msec
	jsr	_fmul
	addq.w	&8,%sp
	movm.l	%d0-%d1,(%sp)
	pea	stat12
	pea	stat6
	jsr	_sprintf
	lea	16(%sp),%sp
	mov.b	%d4,stat6+8		# put in "n", "u", or "m"
	bra.w	stat35	
stat25:	cmp.l	%d0,&0x404E0000		# compare with 60.0
	bcc.b	stat26
	movm.l	%d0-%d1,-(%sp)
	pea	stat11
	pea	stat6
	jsr	_sprintf		# dd.ddd sec
	lea	16(%sp),%sp
	bra.w	stat35
stat26:	cmp.l	%d0,&0x41812A87		# check for overflow
	bhi.b	stat27
	cmp.l	%d0,&0xFFFDF3B6
	bcs.b	stat28
stat27:	pea	stat14			# "time overflow"
	mov.l	file,-(%sp)
	jsr	_fprintf
	addq.w	&8,%sp
	bra.w	stat37
stat28:	mov.l	%d0,%d2			# move time to d2-d3
	mov.l	%d1,%d3
	mov.l	&0xFFFFF,%d7		# mantissa mask
	and.l	%d7,%d2			# isolate mantissa
	addq.l	&1,%d7			# hidden bit mask
	or.l	%d7,%d2			# add hidden bit
	mov.l	%d0,%d4			# move exponent
	swap	%d4			# isolate exponent
	lsr.w	&4,%d4
	mov.l	%d4,%a0			# save exponent
	sub.w	&0x404,%d4		# create counter
	mov.l	&0x001E0000,%d5		# mantissa for 60*2^n in d5-d6
	movq	&0,%d6
stat29:	sub.l	%d6,%d3			# subtract 60*2^n from time
	subx.l	%d5,%d2
	bcc.b	stat30			# error?
	add.l	%d6,%d3			# add it back
	addx.l	%d5,%d2
stat30:	lsr.l	&1,%d5			# now try 60*2^(n-1)
	roxr.l	&1,%d6
	dbra	%d4,stat29		# until 60*2^n < 60
	mov.l	%d2,%d4			# remainder (seconds) in d2-d3
	or.l	%d3,%d4			# check for 0.0
	beq.b	stat33
	mov.l	%a0,%d5			# new exponent
stat31:	cmp.l	%d2,%d7			# normalized?
	bcc.b	stat32
	subq.w	&1,%d5			# decrement exponent
	add.l	%d3,%d3			# shift mantissa left
	addx.l	%d2,%d2
	bra.b	stat31
stat32:	eor.l	%d7,%d2			# remove hidden bit
	lsl.w	&4,%d5			# move exponent into place
	swap	%d5
	clr.w	%d5
	or.l	%d5,%d2			# put exponent field into number
	movm.l	%d0-%d3,-(%sp)		# minutes*60 = total seconds - seconds
	jsr	_fsub
	lea	16(%sp),%sp
stat33:	movm.l	%d0-%d1,-(%sp)		# convert minutes*60 to double
	jsr	_fix
	movq	&60,%d5
	movm.l	%d0/%d5,(%sp)
	jsr	_uldiv			# convert to minutes
	addq.w	&8,%sp
	cmp.l	%d0,%d5			# >= 60 minutes?
	bcc.b	stat34
	movm.l	%d0/%d2-%d3,-(%sp)	# "dd min dd.ddd sec"
	pea	stat10
	pea	stat6
	jsr	_sprintf
	lea	20(%sp),%sp
	bra.b	stat35
stat34:	mov.l	%d0,%d4			# save minutes
	movm.l	%d0/%d5,-(%sp)
	jsr	_ulrem			# minutes % 60
	exg	%d0,%d4
	movm.l	%d0/%d5,(%sp)
	jsr	_uldiv
	movm.l	%d2-%d3,(%sp)
	mov.l	%d4,-(%sp)
	mov.l	%d0,-(%sp)
	pea	stat9
	pea	stat6
	jsr	_sprintf		# "dd hr dd min dd.ddd sec"
	lea	24(%sp),%sp
stat35:	lea	stat6,%a0		# buffer pointer
	movq	&32,%d0			# blank
stat36:	cmp.b	%d0,(%a0)+		# skip over leading blanks
	beq.b	stat36
	subq.w	&1,%a0			# first non-blank
	pea	(%a0)			# part of buffer to print
	pea	stat13
	mov.l	file,-(%sp)
	jsr	_fprintf
	lea	12(%sp),%sp
stat37:	space	0

*/

#include "fizz.h"

static char *stat_code[] = {
    "\tdata",
    "\tlalign\t4",
    "stat1:\tspace\t8",			/* total time end */
    "stat2:\tspace\t8",			/* total time start */
    "stat3:\tspace\t8",			/* overhead time end */
    "stat4:\tspace\t8",			/* overhead time start */
    "stat5:\tspace\t8",			/* timezone dummy argument */
    "stat6:\tspace\t28",		/* print buffer */
    "stat7:\tbyte\t\"avg. time:\",9,0",	/* print field name */
    "stat8:\tbyte\t\"0.0 sec\",10,10,0",/* for <1.0 nanosecond */
    "stat9:\tbyte\t\"%4d hr \"",	/* stat9, stat10, & stat11  */
    "stat10:\tbyte\t\"%2d min \"",	/*      must be consecutive */
    "stat11:\tbyte\t\"%6.3f sec\",10,10,0",
    "stat12:\tbyte\t\"%7.3f *sec\",10,10,0",/* for "nsec", "usec", and "msec" */
    "stat13:\tbyte\t\"%s\",0",		/* for printing print buffer */
    "stat14:\tbyte\t\"time overflow\",10,10,0",	/* for >= 10000 hours */
    "\ttext",
    "\tpea\tstat7",
    "\tmov.l\tfile,-(%sp)",
    "\tjsr\t_fprintf",			/* print field name */
    "\taddq.w\t&8,%sp",
    "\tlea\tstat1,%a0",			/* pointer to time values */
    "\tmov.l\t&1000000,%d0",		/* microseconds per second */
    "\tmovm.l\t(%a0)+,%d2-%d5",		/* get total time values */
    "\tsub.l\t%d5,%d3",			/* subtract microseconds */
    "\tbcc.b\tstat15",			/* need to borrow a second? */
    "\tadd.l\t%d0,%d3",			/* yes: adjust microseconds */
    "\tsubq.l\t&1,%d2",			/* one less second */
    "stat15:\tsub.l\t%d4,%d2",		/* subtract secs (d2-d3 is total time) */
    "\tmovm.l\t(%a0),%d4-%d7",		/* get overhead time values */
    "\tsub.l\t%d7,%d5",			/* subtract microseconds */
    "\tbcc.b\tstat16",			/* need to borrow a second? */
    "\tadd.l\t%d0,%d5",			/* yes: adjust microseconds */
    "\tsubq.l\t&1,%d4",			/* one less second */
    "stat16:\tsub.l\t%d6,%d4",		/* subtract secs (d4-d5 is ovhd time) */
    "\tsub.l\t%d5,%d3",			/* subtract ovhd from total time */
    "\tbcc.b\tstat17",			/* need to borrow? */
    "\tadd.l\t%d0,%d3",			/* yes: adjust microseconds */
    "\tsubq.l\t&1,%d2",			/* one less second */
    "stat17:\tsub.l\t%d4,%d2",		/* sub secs (d2-d3 is result time) */
    "\tbcs.b\tstat18",			/* in case of quantization problem */
    "\tmov.l\t%d3,-(%sp)",		/* convert usecs to double */
    "\tjsr\t_floatu",
    "\tmov.l\t%d1,(%sp)",
    "\tmov.l\t%d0,-(%sp)",
    "\tmov.l\t&0xA0B5ED8D,-(%sp)",	/* 1E-6 */
    "\tmov.l\t&0x3EB0C6F7,-(%sp)",
    "\tjsr\t_fmul",			/* usecs*1E-6 */
    "\taddq.w\t&8,%sp",
    "\tmovm.l\t%d0-%d1,(%sp)",
    "\tmov.l\t%d2,-(%sp)",
    "\tjsr\t_floatu",			/* convert secs to double */
    "\tmov.l\t%d1,(%sp)",
    "\tmov.l\t%d0,-(%sp)",
    "\tjsr\t_fadd",			/* total number of seconds */
    "\tmov.l\t&ILOW,%d5",		/* double precision representation */
    "\tmov.l\t&IHIGH,%d4",		/*     of the number of iterations */
    "\tmovm.l\t%d0-%d1/%d4-%d5,(%sp)",
    "\tjsr\t_fdiv",
    "\tlea\t16(%sp),%sp",		/* d0-d1 is average time */
    "\tcmp.l\t%d0,&0x3E112E0B",		/* compare with 1E-9 */
    "\tbgt.b\tstat19",
    "\tbne.b\tstat18",
    "\tcmp.l\t%d1,&0xE826D695",
    "\tbcc.b\tstat19",
    "stat18:\tpea\tstat8",		/* < 1 nsec; print "0.0 sec" */
    "\tmov.l\tfile,-(%sp)",
    "\tjsr\t_fprintf",
    "\taddq.w\t&8,%sp",
    "\tbra.w\tstat37",
    "stat19:\tcmp.l\t%d0,&0x3EB0C6F7",	/* compare with 1E-6 */
    "\tbhi.b\tstat21",
    "\tbne.b\tstat20",
    "\tcmp.l\t%d1,&0xA0B5ED8D",
    "\tbcc.b\tstat21",
    "stat20:\tmov.l\t&0x41CDCD65,%d2",	/* upper four bytes of 1E9 */
    "\tmovq\t&'n,%d4",			/* "n" for "nsec" */
    "\tbra.b\tstat24",
    "stat21:\tcmp.l\t%d0,&0x3F50624D",	/* compare with 1E-3 */
    "\tbhi.b\tstat23",
    "\tbne.b\tstat22",
    "\tcmp.l\t%d1,&0xD2F1A9FC",
    "\tbcc.b\tstat23",
    "stat22:\tmov.l\t&0x412E8480,%d2",	/* upper four bytes of 1E6 */
    "\tmovq\t&'u,%d4",			/* "u" for "usec" */
    "\tbra.b\tstat24",
    "stat23:\tcmp.l\t%d0,&0x3FF00000",	/* compare with 1.0 */
    "\tbcc.b\tstat25",
    "\tmov.l\t&0x408F4000,%d2",		/* upper four bytes of 1E3 */
    "\tmovq\t&'m,%d4",			/* "m" for "msec" */
    "stat24:\tmovq\t&0,%d3",
    "\tmovm.l\t%d0-%d3,-(%sp)",		/* convert nsec, usec, or msec */
    "\tjsr\t_fmul",
    "\taddq.w\t&8,%sp",
    "\tmovm.l\t%d0-%d1,(%sp)",
    "\tpea\tstat12",
    "\tpea\tstat6",
    "\tjsr\t_sprintf",
    "\tlea\t16(%sp),%sp",
    "\tmov.b\t%d4,stat6+8",		/* put in "n", "u", or "m" */
    "\tbra.w\tstat35",	
    "stat25:\tcmp.l\t%d0,&0x404E0000",	/* compare with 60.0 */
    "\tbcc.b\tstat26",
    "\tmovm.l\t%d0-%d1,-(%sp)",
    "\tpea\tstat11",
    "\tpea\tstat6",
    "\tjsr\t_sprintf",			/* dd.ddd sec */
    "\tlea\t16(%sp),%sp",
    "\tbra.w\tstat35",
    "stat26:\tcmp.l\t%d0,&0x41812A87",	/* check for overflow */
    "\tbhi.b\tstat27",
    "\tcmp.l\t%d0,&0xFFFDF3B6",
    "\tbcs.b\tstat28",
    "stat27:\tpea\tstat14",		/* "time overflow" */
    "\tmov.l\tfile,-(%sp)",
    "\tjsr\t_fprintf",
    "\taddq.w\t&8,%sp",
    "\tbra.w\tstat37",
    "stat28:\tmov.l\t%d0,%d2",		/* move time to d2-d3 */
    "\tmov.l\t%d1,%d3",
    "\tmov.l\t&0xFFFFF,%d7",		/* mantissa mask */
    "\tand.l\t%d7,%d2",			/* isolate mantissa */
    "\taddq.l\t&1,%d7",			/* hidden bit mask */
    "\tor.l\t%d7,%d2",			/* add hidden bit */
    "\tmov.l\t%d0,%d4",			/* move exponent */
    "\tswap\t%d4",			/* isolate exponent */
    "\tlsr.w\t&4,%d4",
    "\tmov.l\t%d4,%a0",			/* save exponent */
    "\tsub.w\t&0x404,%d4",		/* create counter */
    "\tmov.l\t&0x001E0000,%d5",		/* mantissa for 60*2^n in d5-d6 */
    "\tmovq\t&0,%d6",
    "stat29:\tsub.l\t%d6,%d3",		/* subtract 60*2^n from time */
    "\tsubx.l\t%d5,%d2",
    "\tbcc.b\tstat30",			/* error? */
    "\tadd.l\t%d6,%d3",			/* add it back */
    "\taddx.l\t%d5,%d2",
    "stat30:\tlsr.l\t&1,%d5",		/* now try 60*2^(n-1) */
    "\troxr.l\t&1,%d6",
    "\tdbra\t%d4,stat29",		/* until 60*2^n < 60 */
    "\tmov.l\t%d2,%d4",			/* remainder (seconds) in d2-d3 */
    "\tor.l\t%d3,%d4",			/* check for 0.0 */
    "\tbeq.b\tstat33",
    "\tmov.l\t%a0,%d5",			/* new exponent */
    "stat31:\tcmp.l\t%d2,%d7",		/* normalized? */
    "\tbcc.b\tstat32",
    "\tsubq.w\t&1,%d5",			/* decrement exponent */
    "\tadd.l\t%d3,%d3",			/* shift mantissa left */
    "\taddx.l\t%d2,%d2",
    "\tbra.b\tstat31",
    "stat32:\teor.l\t%d7,%d2",		/* remove hidden bit */
    "\tlsl.w\t&4,%d5",			/* move exponent into place */
    "\tswap\t%d5",
    "\tclr.w\t%d5",
    "\tor.l\t%d5,%d2",			/* put exponent field into number */
    "\tmovm.l\t%d0-%d3,-(%sp)",		/* minutes*60 = total secs - secs */
    "\tjsr\t_fsub",
    "\tlea\t16(%sp),%sp",
    "stat33:\tmovm.l\t%d0-%d1,-(%sp)",	/* convert minutes*60 to double */
    "\tjsr\t_fix",
    "\tmovq\t&60,%d5",
    "\tmovm.l\t%d0/%d5,(%sp)",
    "\tjsr\t_uldiv",			/* convert to minutes */
    "\taddq.w\t&8,%sp",
    "\tcmp.l\t%d0,%d5",			/* >= 60 minutes? */
    "\tbcc.b\tstat34",
    "\tmovm.l\t%d0/%d2-%d3,-(%sp)",	/* "dd min dd.ddd sec" */
    "\tpea\tstat10",
    "\tpea\tstat6",
    "\tjsr\t_sprintf",
    "\tlea\t20(%sp),%sp",
    "\tbra.b\tstat35",
    "stat34:\tmov.l\t%d0,%d4",		/* save minutes */
    "\tmovm.l\t%d0/%d5,-(%sp)",
    "\tjsr\t_ulrem",			/* minutes % 60 */
    "\texg\t%d0,%d4",
    "\tmovm.l\t%d0/%d5,(%sp)",
    "\tjsr\t_uldiv",
    "\tmovm.l\t%d2-%d3,(%sp)",
    "\tmov.l\t%d4,-(%sp)",
    "\tmov.l\t%d0,-(%sp)",
    "\tpea\tstat9",
    "\tpea\tstat6",
    "\tjsr\t_sprintf",			/* "dd hr dd min dd.ddd sec" */
    "\tlea\t24(%sp),%sp",
    "stat35:\tlea\tstat6,%a0",		/* buffer pointer */
    "\tmovq\t&32,%d0",			/* blank */
    "stat36:\tcmp.b\t%d0,(%a0)+",	/* skip over leading blanks */
    "\tbeq.b\tstat36",
    "\tsubq.w\t&1,%a0",			/* first non-blank */
    "\tpea\t(%a0)",			/* part of buffer to print */
    "\tpea\tstat13",
    "\tmov.l\tfile,-(%sp)",
    "\tjsr\t_fprintf",
    "\tlea\t12(%sp),%sp",
    "stat37:\tspace\t0",
};

void Dprint_stat_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;
    union { double d; unsigned long l[2]; } n;

    n.d = (double) gIterations;
    print(output, "\tset\tIHIGH,%#X\n\tset\tILOW,%#X\n",n.l[0],n.l[1]);
    for (count = 0; count < sizeof(stat_code)/sizeof(char *); count++)
        print(output, "%s\n", stat_code[count]);
    return;
}
