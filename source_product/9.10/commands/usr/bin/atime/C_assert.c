/*

    Cprint_assertion_code()

    Create code to perform an assertion. For example:

	assert.b   sign,%d0
	assert.w   point,4(%a2,%d3.w)
	assert.l   frame,_frame


    Note:
	<ea> = effective address
	{index} = assert number; the first is 0, the second 4, the
		third 8, etc.

		mov.w	%cc,___ZCC	# "file", line {line_number}
    #if the size is long
		mov.l	<ea>,___ZEA	# {instruction}
    #else if the size is word
		mov.w	<ea>,___ZEA	# {instruction}
    #else
		mov.b	<ea>,___ZEA	# {instruction}
    #endif
		mov.w	___ZCC,-(%sp)	# save %cc

    #if the size is long
		mov.l	___ZEA,-(%sp)	# actual value
		mov.w	&'l,-(%sp)	# size
    #else
		lea	-10(%sp),%sp	# space for %d0, size, value
		mov.l	%d0,(%sp)	# save %d0
     #if the size is byte
		mov.w	&'b,4(%sp)	# store size
		mov.b	___ZEA,%d0	# get actual value
		ext.w	%d0		# sign extend to 16 bits
     #else
		mov.w	&'w,4(%sp)	# store size
		mov.w	___ZEA,%d0	# get actual value
     #endif
		ext.l	%d0		# sign extend to 32 bits
		mov.l	%d0,6(%sp)	# store actual value
		mov.l	(%sp)+,%d0	# restore %d0
    #endif

		pea	{index}		# index parameter

    #if mode is assertion listing
		jsr	___Zlist	# list asserted value
    #else
		jsr	___Zassert	# check assertion
    #endif

		lea	10(%sp),%sp	# remove parameters
		mov.w	(%sp)+,%cc	# restore %cc

*/

#include "fizz.h"

void Cprint_assertion_code(size, index, effective_address)
short size;
unsigned long index;
char *effective_address;
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    char *buffer;

    print(output, "\tmov.w\t%%cc,___ZCC\t# \"%s\", line %u\n", gFile, gLine);
    buffer = compact_instruction();
    print(output, "\tmov.%c\t%s,___ZEA\t# %s\n", size == LONG ? 'l' : size ==
      WORD ? 'w' : 'b', effective_address, buffer);
    free(buffer);
    print(output, "\tmov.w\t___ZCC,-(%%sp)\n");
    if (size == LONG) {
	print(output, "\tmov.l\t___ZEA,-(%%sp)\n");
	print(output, "\tmov.w\t&'l,-(%%sp)\n");
    } else {
	print(output, "\tlea\t-10(%%sp),%%sp\n");
	print(output, "\tmov.l\t%%d0,(%%sp)\n");
	if (size == BYTE) {
	    print(output, "\tmov.w\t&'b,4(%%sp)\n");
	    print(output, "\tmov.b\t___ZEA,%%d0\n");
	    print(output, "\text.w\t%%d0\n");
	} else {
	    print(output, "\tmov.w\t&'w,4(%%sp)\n");
	    print(output, "\tmov.w\t___ZEA,%%d0\n");
	};
	print(output, "\text.l\t%%d0\n");
	print(output, "\tmov.l\t%%d0,6(%%sp)\n");
	print(output, "\tmov.l\t(%%sp)+,%%d0\n");
    };
    print(output, "\tpea\t%d\n", index * 4);
    if (gMode == ASSERTION_LISTING)
	print(output, "\tjsr\t___Zlist\n");
    else
	print(output, "\tjsr\t___Zassert\n");
    print(output, "\tlea\t10(%%sp),%%sp\n");
    print(output, "\tmov.w\t(%%sp)+,%%cc\n");

    return;
}
