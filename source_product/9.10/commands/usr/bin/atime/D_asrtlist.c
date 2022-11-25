/*

    Dprint_assertion_listing_routine()

    Create a routine which will be called to perform the actual assertion 
    listing.

	Note:    {namesize} = maximum size of all dynamic parameter 
			      names and assertion names; the output
			      format is adjusted to accomodate various
			      name lengths (see printwidth())

		 Each entry in the assertion tables is six bytes long.
		 The first 16 bits contains size information:
			'b' = byte
			'w' = word
			'l' = long
			-1  = size is not defined in assertion file
			 0  = this assert instruction has already been
			      executed for this dataset
		 The next 32 bits contains the correct assertion value,
		 sign-extended if necessary.

	#if there are assertions
		data
	Zl1:	byte	9,9,"%-{namesize}s%12d.%c",10
	 #if there is an assertion file
	Zl2:	byte	9,9,"%-{namesize}s%12d%c%12d.%c",10
	Zl3:	byte	9,9,"%-{namesize}s%12d.%c       MISSING",10
	 #endif
		text
		global	___Zlist
	___Zlist:
	 #if there is an assertion file
		link	%a6,&-20	# space for regs & format specifier
	 #else
		link	%a6,&-16	# space for regs
	 #endif
		movm.l	%d0-%d1/%a0-%a1,(%sp)	# save registers...if we don't
					# smash 'em ourselves, fprintf() will
	 #if there is an assertion file
		mov.l	&Zl1,-4(%a6)	# assume this format
	 #endif
		mov.l	8(%a6),%a0	# assertion index
		mov.l	%a0,%d0		# because this index references a table
		lsr.l	&1,%d0		# with 4-byte elements, we must convert
		add.l	%d0,%a0		# it to one for 6-byte elements
	 #if there is more than one data set
		add.l	cdsetver,%a0	# table base (dynamic)
	 #else
		add.l	&assert,%a0	# table base (static)
	 #endif
		movq	&0,%d0
		mov.w	(%a0),%d0	# get assertion size
		beq.b	Zl7		# already asserted?
	 #if there is an assertion file
		bpl.b	Zl4		# size undefined?
		clr.w	(%a0)		# yes; mark this assertion
		mov.l	&Zl3,-4(%a6)	# use "MISSING" format
		bra.b	Zl6
	Zl4:	clr.w	(%a0)+		# mark assertion
		mov.l	(%a0),%d1	# get value
		cmp.w	%d0,12(%a6)	# compare sizes
		bne.b	Zl5
		cmp.l	%d1,14(%a6)	# compare values
		beq.b	Zl6
	Zl5:	mov.l	%d0,-(%sp)	# size and/or value mismatch
		mov.l	%d1,-(%sp)	# push correct value and size
		mov.l	&Zl2,-4(%a6)	# use value mismatch format
	Zl6:	
	 #else
		clr.w	(%a0)			# just mark this assertion
	 #endif
		mov.w	12(%a6),%d0		# get asserted size
		mov.l	%d0,-(%sp)
		mov.l	14(%a6),-(%sp)		# get asserted value
	 #if there is more than one assertion
		lea	aname,%a0		# get assertion name
		add.l	8(%a6),%a0		# depends on assertion index
		mov.l	(%a0),-(%sp)
	 #else
		pea	aname			# push the only name
	 #endif
	 #if there is an assertion file
		mov.l	-4(%a6),-(%sp)		# push the appropriate format
	 #else
		pea	Zl1			# only one possible format
	 #endif
		mov.l	file,-(%sp)		# where to print it
		jsr	_fprintf		# notify the user
	 #if there is an assertion file
		lea	-20(%a6),%sp		# clean off the stack
	 #else
		lea	-16(%a6),%sp		# clean off the stack
	 #endif
	Zl7:	movm.l	(%sp),%d0-%d1/%a0-%a1	# restore the registers
		unlk	%a6
		rts
	#endif

*/

#include "fizz.h"

static char *zlist1[] = {
	"\ttext",
	"\tglobal\t___Zlist",
	"___Zlist:",
};

static char *zlist2[] = {
	"\tmov.l\t8(%a6),%a0",
	"\tmov.l\t%a0,%d0",
	"\tlsr.l\t&1,%d0",
	"\tadd.l\t%d0,%a0",
};

static char *zlist3[] = {
	"\tmovq\t&0,%d0",
	"\tmov.w\t(%a0),%d0",
	"\tbeq.b\tZl7",
};

static char *zlist4[] = {
	"\tbpl.b\tZl4",
	"\tclr.w\t(%a0)",
	"\tmov.l\t&Zl3,-4(%a6)",
	"\tbra.b\tZl6",
	"Zl4:\tclr.w\t(%a0)+",
	"\tmov.l\t(%a0),%d1",
	"\tcmp.w\t%d0,12(%a6)",
	"\tbne.b\tZl5",
	"\tcmp.l\t%d1,14(%a6)",
	"\tbeq.b\tZl6",
	"Zl5:\tmov.l\t%d0,-(%sp)",
	"\tmov.l\t%d1,-(%sp)",
	"\tmov.l\t&Zl2,-4(%a6)",
	"Zl6:",
};

static char *zlist5[] = {
	"\tmov.w\t12(%a6),%d0",
	"\tmov.l\t%d0,-(%sp)",
	"\tmov.l\t14(%a6),-(%sp)",
};

static char *zlist6[] = {
	"\tlea\taname,%a0",
	"\tadd.l\t8(%a6),%a0",
	"\tmov.l\t(%a0),-(%sp)",
};

static char *zlist7[] = {
	"Zl7:\tmovm.l\t(%sp),%d0-%d1/%a0-%a1",
	"\tunlk\t%a6",
	"\trts",
};

void Dprint_assertion_listing_routine()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;
    unsigned long width;

    if (!gAssertionCount) return;
    print(output, "\tdata\n");
    width = printwidth();
    print(output, "Zl1:\tbyte\t9,9,\"%%-%us%%12d.%%c\",10,0\n", width);
    if (gfAssertionFile) {
	print(output, "Zl2:\tbyte\t9,9,\"%%-%us%%12d.%%c%%12d.%%c\",10,0\n", 
	  width);
	print(output, "Zl3:\tbyte\t9,9,\"%%-%us%%12d.%%c       MISSING\",10,0\n"
	  , width);
    };
    for (count = 0; count < sizeof(zlist1)/sizeof(char *); count++)
	print(output, "%s\n", zlist1[count]);
    print(output, "\tlink\t%%a6,&-%d\n", gfAssertionFile ? 20 : 16);
    print(output, "\tmovm.l\t%%d0-%%d1/%%a0-%%a1,(%%sp)\n");
    if (gfAssertionFile) print(output, "\tmov.l\t&Zl1,-4(%%a6)\n");
    for (count = 0; count < sizeof(zlist2)/sizeof(char *); count++)
	print(output, "%s\n", zlist2[count]);
    print(output, "\tadd.l\t%s,%%a0\n", 
      gDatasetCount > 1 ? "cdsetver" : "&assert");
    for (count = 0; count < sizeof(zlist3)/sizeof(char *); count++)
	print(output, "%s\n", zlist3[count]);
    if (gfAssertionFile) {
        for (count = 0; count < sizeof(zlist4)/sizeof(char *); count++)
	    print(output, "%s\n", zlist4[count]);
    } else print(output, "\tclr.w\t(%%a0)\n");
    for (count = 0; count < sizeof(zlist5)/sizeof(char *); count++)
	print(output, "%s\n", zlist5[count]);
    if (gAssertionCount > 1) {
        for (count = 0; count < sizeof(zlist6)/sizeof(char *); count++)
	    print(output, "%s\n", zlist6[count]);
    } else print(output, "\tpea\taname\n");
    if (gfAssertionFile) print(output, "\tmov.l\t-4(%%a6),-(%%sp)\n");
    else print(output, "\tpea\tZl1\n");
    print(output, "\tmov.l\tfile,-(%%sp)\n");
    print(output, "\tjsr\t_fprintf\n");
    print(output, "\tlea\t-%d(%%a6),%%sp\n", gfAssertionFile ? 20 : 16);
    for (count = 0; count < sizeof(zlist7)/sizeof(char *); count++)
        print(output, "%s\n", zlist7[count]);

    return;
}
