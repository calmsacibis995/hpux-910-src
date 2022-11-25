/*

    Dprint_assertion_routine()

    Create a routine to do assertions for performance analysis or
    execution profiling.  Three parameters are passed to this created
    routine:  a long word offset into the assertion table, a word size
    ('b'=byte; 'w'=word; 'l'=long), and a value which is converted to
    long.

    #if there are assertions
		global	___Zassert

		data

     #if there are data sets
	Za1:	byte	"ERROR: missing value for %s.%c (dataset: %s)",10,0
	Za2:	byte	"ERROR: size mismatch for %s.%c (dataset: %s), expected \".%c\"",10,0
	Za3:	byte	"ERROR: %s = %d.%c (dataset: %s), expected %d.%c",10,0
     #else
	Za1:	byte	"ERROR: missing value for %s.%c",10,0
	Za2:	byte	"ERROR: size mismatch for %s.%c, expected %c",10,0
	Za3:	byte	"ERROR: %s = %d.%c, expected %d.%c",10,0
     #endif

		text
	___Zassert:
		link	%a6,&-16		# space for registers
		movm.l	%d0-%d1/%a0-%a1,(%sp)	# save registers
		mov.l	8(%a6),%a0		# get assertion table offset
		mov.l	%a0,%d1			# convert from 4*index to
		lsr.l	&1,%d1			#    6*index (entries are
		add.l	%d1,%a0			#    six bytes each)

     #if there is more than one data set
		add.l	cdsetver,%a0		# point into table
     #else
		add.l	&assert,%a0
     #endif
		movq	&0,%d0
		mov.w	(%a0),%d0		# get size
		beq.b	Za4			# assert already executed?
		bmi.b	Za5			# no assertion data?
		clr.w	(%a0)+			# mark as executed
		cmp.w	%d0,12(%a6)		# correct size?
		bne.w	Za6
		mov.l	(%a0),%d1		# get correct value
		cmp.l	%d1,14(%a6)		# is asserted value correct?
		bne.w	Za7
	Za4:	movm.l	(%sp),%d0-%d1/%a0-%a1	# restore registers
		unlk	%a6
		rts

	#
	# No assertion data from an assert file
	#
	Za5:	clr.w	(%a0)			# mark as executed

     #if there are data sets
      #if there is more than one data set
		lea	dname,%a0		#    +
       #if {datasets} > 8192			#    |
		add.l	cdset,%a0		#    |
       #else					#    |
		add.w	cdset,%a0		#  dname[dataset]
       #endif					#    |
		mov.l	(%a0),-(%sp)		#    |
      #else					#    |
		pea	dname			#    |
      #endif					#    |
     #endif					#    +

		mov.w	12(%a6),%d0		#  size
		mov.l	%d0,-(%sp)

     #if there is more than one assertion	#    +
		lea	aname,%a0		#    |
		add.l	8(%a6),%a0		#    |
		mov.l	(%a0),-(%sp)		#  aname[index]
     #else					#    |
		pea	aname			#    |
     #endif					#    +

		pea	Za1			#  format
		mov.l	file,-(%sp)		#  file
		lea	-20(%a6),%a0		#  parameters are now
		mov.l	(%a0),-(%sp)		#     duplicated because
		mov.l	-(%a0),-(%sp)		#     they will be printed
		mov.l	-(%a0),-(%sp)		#     to both the output
     #if there are data sets			#     file and to stderr
		mov.l	-(%a0),-(%sp)
     #endif
		pea	__iob+32		# stderr
		jsr	_fprintf		# fprintf(stderr,Za1,
						#   aname[index],
						#   size{,dname[dataset]})
     #if there are data sets
		lea	20(%sp),%sp		# clean off parameters
     #else
		lea	16(%sp),%sp
     #endif
		jsr	_fprintf		# fprintf(file,Za1,aname[index],
						#   size{,dname[dataset]})
     #if there are data sets
		lea	20(%sp),%sp
     #else
		lea	16(%sp),%sp
     #endif
		st	vererr			# flag an error
		movm.l	(%sp),%d0-%d1/%a0-%a1
		unlk	%a6
		rts

	#
	# size mismatch
	#
	Za6:	mov.l	%d0,-(%sp)		# parameter: correct size

     #if there are data sets			#    +
      #if there is more than one data set	#    |
		lea	dname,%a0		#    |
       #if {datasets} > 8192			#    |
		add.l	cdset,%a0		#    |
       #else					#  dname[dataset]
		add.w	cdset,%a0		#    |
       #endif					#    |
		mov.l	(%a0),-(%sp)		#    |
      #else					#    |
		pea	dname			#    |
      #endif					#    |
     #endif					#    +

		mov.w	12(%a6),%d0		# asserted size
		mov.l	%d0,-(%sp)

     #if there is more than one assertion	#    +
		lea	aname			#    |
		add.l	8(%a6),%a0		#    |
		mov.l	(%a0),-(%sp)		#  aname[index] 
     #else					#    |
		pea	aname			#    |
     #endif					#    +

		pea	Za2			#  format
		mov.l	file,-(%sp)		#  file
		lea	-20(%a6),%a0		# the parameters are
		mov.l	(%a0),-(%sp)		#   duplicated because the
		mov.l	-(%a0),-(%sp)		#   is printed to both the
		mov.l	-(%a0),-(%sp)		#   output file and to 
		mov.l	-(%a0),-(%sp)		#   stderr
     #if there are data sets
		mov.l	-(%a0),-(%sp)
     #endif
		pea	__iob+32		# stderr
		jsr	_fprintf		# fprintf(stderr,Za2,
						#  aname[index],
						#  size,{dname[dataset],}
						#  correct size)
     #if there are data sets
		lea	24(%sp),%sp		# clean off parameters
     #else
		lea	20(%sp),%sp
     #endif
		jsr	_fprintf		# fprintf(file,Za2,aname[index],
						#  size,{dname[dataset].}
						#  correct size)
     #if there are data sets
		lea	24(%sp),%sp		# clean off parameters
     #else
		lea	20(%sp),%sp
     #endif
		st	vererr			# flag an error
		movm.l	(%sp),%d0-%d1/%a0-%a1	# restore registers
		unlk	%a6
		rts

	#
	# value mismatch
	#
	Za7:	mov.l	%d0,-(%sp)		# correct size
		mov.l	%d1,-(%sp)		# correct value

     #if there are data sets			#    +
      #if there is more than one data set	#    |
		lea	dname,%a0		#    |
       #if {datasets} > 8192			#    |
		add.l	cdset,%a0		#    |
       #else					#    |
		add.w	cdset,%a0		# dname[dataset]
       #endif					#    |
		mov.l	(%a0),-(%sp)		#    |
      #else					#    |
		pea	dname			#    |
      #endif					#    |
     #endif					#    +

		mov.l	12(%a6),%d0		# asserted size
		mov.l	%d0,-(%sp)

		mov.l	14(%a6),-(%sp)		# asserted value

     #if there is more than one assertion	#    +
		lea	aname			#    |
		add.l	8(%a6),%a0		#    |
		mov.l	(%a0),-(%sp)		# aname[index]
     #else					#    |
		pea	aname			#    |
     #endif					#    +

		pea	Za3			# format
		mov.l	file,-(%sp)		# file
		lea	-20(%a6),%a0		# parameters are duplicated
		mov.l	(%a0),-(%sp)		#   because the message is
		mov.l	-(%a0),-(%sp)		#   printed to both stderr
		mov.l	-(%a0),-(%sp)		#   and to the output file
		mov.l	-(%a0),-(%sp)
		mov.l	-(%a0),-(%sp)
		mov.l	-(%a0),-(%sp)
     #if there are data sets
		mov.l	-(%a0),-(%sp)
     #endif
		pea	__iob+32		# stderr
		jsr	_fprintf		# fprintf(stderr,Za3
						#   aname[index],asserted
						#   value, asserted size,
						#   {dname[dataset],} correct
						#   value, correct size)
     #if there are data sets
		lea	32(%sp),%sp		# clean off parameters
     #else
		lea	28(%sp),%sp
     #endif
		jsr	_fprintf		# fprintf(file,Za3
						#   aname[index],asserted
						#   value, asserted size,
						#   {dname[dataset],} correct
						#   value, correct size)
     #if there are data sets
		lea	32(%sp),%sp		# clean off parameters
     #else
		lea	28(%sp),%sp
     #endif
		st	vererr			# flag an error
		movm.l	(%sp),%d0-%d1/%a0-%a1   # restore registers
		unlk	%a6
		rts
    #endif

*/

#include "fizz.h"

static char *routine1[] = {
	"Za1:\tbyte\t\"ERROR: missing value for %s.%c (dataset: %s)\",10,0",
	"Za2:\tbyte\t\"ERROR: size mismatch for %s.%c (dataset: %s), expected \\\".%c\\\"\",10,0",
	"Za3:\tbyte\t\"ERROR: %s = %d.%c (dataset: %s), expected %d.%c\",10,0",
};

static char *routine2[] = {
	"Za1:\tbyte\t\"ERROR: missing value for %s.%c\",10,0",
	"Za2:\tbyte\t\"ERROR: size mismatch for %s.%c, expected \\\".%c\\\"\",10,0",
	"Za3:\tbyte\t\"ERROR: %s = %d.%c, expected %d.%c\",10,0",
};

static char *routine3[] = {
	"\ttext",
	"___Zassert:",
	"\tlink\t%a6,&-16",
	"\tmovm.l\t%d0-%d1/%a0-%a1,(%sp)",
	"\tmov.l\t8(%a6),%a0",
	"\tmov.l\t%a0,%d1",
	"\tlsr.l\t&1,%d1",
	"\tadd.l\t%d1,%a0",
};

static char *routine4[] = {
	"\tmovq\t&0,%d0",
	"\tmov.w\t(%a0),%d0",
	"\tbeq.b\tZa4",
	"\tbmi.b\tZa5",
	"\tclr.w\t(%a0)+",
	"\tcmp.w\t%d0,12(%a6)",
	"\tbne.w\tZa6",
	"\tmov.l\t(%a0),%d1",
	"\tcmp.l\t%d1,14(%a6)",
	"\tbne.w\tZa7",
	"Za4:\tmovm.l\t(%sp),%d0-%d1/%a0-%a1",
	"\tunlk\t%a6",
	"\trts",
	"Za5:\tclr.w\t(%a0)",
};

static char *routine5[] = {
	"\tlea\taname,%a0",
	"\tadd.l\t8(%a6),%a0",
	"\tmov.l\t(%a0),-(%sp)",
};

static char *routine6[] = {
	"\tpea\tZa1",
	"\tmov.l\tfile,-(%sp)",
	"\tlea\t-20(%a6),%a0",
	"\tmov.l\t(%a0),-(%sp)",
	"\tmov.l\t-(%a0),-(%sp)",
	"\tmov.l\t-(%a0),-(%sp)",
};

static char *routine7[] = {
	"\tst\tvererr",
	"\tmovm.l\t(%sp),%d0-%d1/%a0-%a1",
	"\tunlk\t%a6",
	"\trts",
	"Za6:\tmov.l\t%d0,-(%sp)",
};

static char *routine8[] = {
	"\tpea\tZa2",
	"\tmov.l\tfile,-(%sp)",
	"\tlea\t-20(%a6),%a0",
	"\tmov.l\t(%a0),-(%sp)",
	"\tmov.l\t-(%a0),-(%sp)",
	"\tmov.l\t-(%a0),-(%sp)",
	"\tmov.l\t-(%a0),-(%sp)",
};

static char *routine9[] = {
	"\tst\tvererr",
	"\tmovm.l\t(%sp),%d0-%d1/%a0-%a1",
	"\tunlk\t%a6",
	"\trts",
	"Za7:\tmov.l\t%d0,-(%sp)",
	"\tmov.l\t%d1,-(%sp)",
};

static char *routine10[] = {
	"\tmov.w\t12(%a6),%d0",
	"\tmov.l\t%d0,-(%sp)",
	"\tmov.l\t14(%a6),-(%sp)",
};

static char *routine11[] = {
	"\tpea\tZa3",
	"\tmov.l\tfile,-(%sp)",
	"\tlea\t-20(%a6),%a0",
	"\tmov.l\t(%a0),-(%sp)",
	"\tmov.l\t-(%a0),-(%sp)",
	"\tmov.l\t-(%a0),-(%sp)",
	"\tmov.l\t-(%a0),-(%sp)",
	"\tmov.l\t-(%a0),-(%sp)",
	"\tmov.l\t-(%a0),-(%sp)",
};

static char *routine12[] = {
	"\tst\tvererr",
	"\tmovm.l\t(%sp),%d0-%d1/%a0-%a1",
	"\tunlk\t%a6",
	"\trts",
};

void Dprint_assertion_routine()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (gAssertionCount) {
	print(output, "\tglobal\t___Zassert\n");
	print(output, "\tdata\n");
	if (gDatasetCount)
            for (count = 0; count < sizeof(routine1)/sizeof(char *); count++)
	        print(output, "%s\n", routine1[count]);
	else
            for (count = 0; count < sizeof(routine2)/sizeof(char *); count++)
	        print(output, "%s\n", routine2[count]);
        for (count = 0; count < sizeof(routine3)/sizeof(char *); count++)
	    print(output, "%s\n", routine3[count]);
	print(output, "\tadd.l\t%s,%%a0\n",
	    gDatasetCount > 1 ? "cdsetver" : "&assert");
        for (count = 0; count < sizeof(routine4)/sizeof(char *); count++)
	    print(output, "%s\n", routine4[count]);
	if (gDatasetCount) {
	    if (gDatasetCount > 1) {
		print(output, "\tlea\tdname,%%a0\n");
		print(output, "\tadd.%c\tcdset,%%a0\n",
		  gDatasetCount > 8192 ? 'l' : 'w');
		print(output, "\tmov.l\t(%%a0),-(%%sp)\n");
	    } else print(output, "\tpea\tdname\n");
	};
	print(output, "\tmov.w\t12(%%a6),%%d0\n");
	print(output, "\tmov.l\t%%d0,-(%%sp)\n");
	if (gAssertionCount > 1) 
            for (count = 0; count < sizeof(routine5)/sizeof(char *); count++)
	        print(output, "%s\n", routine5[count]);
        else print(output, "\tpea\taname\n");
        for (count = 0; count < sizeof(routine6)/sizeof(char *); count++)
	    print(output, "%s\n", routine6[count]);
	if (gDatasetCount) print(output, "\tmov.l\t-(%%a0),-(%%sp)\n");
	print(output, "\tpea\t__iob+32\n");
	print(output, "\tjsr\t_fprintf\n");
	print(output, "\tlea\t%s(%%sp),%%sp\n", gDatasetCount ? "20" : "16");
	print(output, "\tjsr\t_fprintf\n");
	print(output, "\tlea\t%s(%%sp),%%sp\n", gDatasetCount ? "20" : "16");
        for (count = 0; count < sizeof(routine7)/sizeof(char *); count++)
	    print(output, "%s\n", routine7[count]);
	if (gDatasetCount) {
	    if (gDatasetCount > 1) {
		print(output, "\tlea\tdname,%%a0\n");
		print(output, "\tadd.%c\tcdset,%%a0\n",
		  gDatasetCount > 8192 ? 'l' : 'w');
		print(output, "\tmov.l\t(%%a0),-(%%sp)\n");
	    } else print(output, "\tpea\tdname\n");
	};
	print(output, "\tmov.w\t12(%%a6),%%d0\n");
	print(output, "\tmov.l\t%%d0,-(%%sp)\n");
	if (gAssertionCount > 1) 
            for (count = 0; count < sizeof(routine5)/sizeof(char *); count++)
	        print(output, "%s\n", routine5[count]);
        else print(output, "\tpea\taname\n");
        for (count = 0; count < sizeof(routine8)/sizeof(char *); count++)
	    print(output, "%s\n", routine8[count]);
	if (gDatasetCount) print(output, "\tmov.l\t-(%%a0),-(%%sp)\n");
	print(output, "\tpea\t__iob+32\n");
	print(output, "\tjsr\t_fprintf\n");
	print(output, "\tlea\t%s(%%sp),%%sp\n", gDatasetCount ? "24" : "20");
	print(output, "\tjsr\t_fprintf\n");
	print(output, "\tlea\t%s(%%sp),%%sp\n", gDatasetCount ? "24" : "20");
        for (count = 0; count < sizeof(routine9)/sizeof(char *); count++)
	    print(output, "%s\n", routine9[count]);
	if (gDatasetCount) {
	    if (gDatasetCount > 1) {
		print(output, "\tlea\tdname,%%a0\n");
		print(output, "\tadd.%c\tcdset,%%a0\n",
		  gDatasetCount > 8192 ? 'l' : 'w');
		print(output, "\tmov.l\t(%%a0),-(%%sp)\n");
	    } else print(output, "\tpea\tdname\n");
	};
        for (count = 0; count < sizeof(routine10)/sizeof(char *); count++)
	    print(output, "%s\n", routine10[count]);
	if (gAssertionCount > 1) 
            for (count = 0; count < sizeof(routine5)/sizeof(char *); count++)
	        print(output, "%s\n", routine5[count]);
        else print(output, "\tpea\taname\n");
        for (count = 0; count < sizeof(routine11)/sizeof(char *); count++)
	    print(output, "%s\n", routine11[count]);
	if (gDatasetCount) print(output, "\tmov.l\t-(%%a0),-(%%sp)\n");
	print(output, "\tpea\t__iob+32\n");
	print(output, "\tjsr\t_fprintf\n");
	print(output, "\tlea\t%s(%%sp),%%sp\n", gDatasetCount ? "32" : "28");
	print(output, "\tjsr\t_fprintf\n");
	print(output, "\tlea\t%s(%%sp),%%sp\n", gDatasetCount ? "32" : "28");
        for (count = 0; count < sizeof(routine12)/sizeof(char *); count++)
	    print(output, "%s\n", routine12[count]);
    };

    return;
}
