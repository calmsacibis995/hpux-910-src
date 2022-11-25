/*

    Dprint_verify_code()

    Create code to perform assertions.

    #if there are assertions

		data
		lalign	4
	ver_a6:	space	4	# for saving %a6 (may be trashed by user code)
	ver_sp:	space	4	# for saving %sp (may be trashed by user code)

     #if mode is assertion listing
	verify1:byte	10,0	# final linefeed for printing a line
     #else
	vererr:	byte	0	# assert error flag - causes abort in the end
     #endif

     #if there is more than one data set
		lalign	4
      #if {datasets} > 8192
	cdset:	space	4	# current dataset index
      #else			#   used for text tables
	cdset:	space	2
      #endif
     #endif

		text

     #if there is more than one data set

      #if there are dynamic parameters
		lea	dsetpar,%a0	# parameter table
		mov.l	(%a0)+,cdsetpar # current table for current dataset
		mov.l	%a0,-4(%a6)	# save pointer to next one
      #endif

		lea	dsetver,%a0	# verify (assertion) table
		mov.l	(%a0)+,cdsetver # current table for current dataset
		mov.l	%a0,-8(%a6)	# save pointer to next one

      #if {datasets} > 65535
		mov.l	&{datasets},-12(%a6)	# dataset counter
      #else
		mov.w	&{datasets},-12(%a6)
      #endif

      #if {datasets} > 8192
		clr.l	cdset		# dataset index
      #else
		clr.w	cdset
      #endif

     #endif

	verify:	
     #if mode is assertion listing
      #if there are data sets

		mov.l	file,-(%sp)		# dump info concerning
						#  dataset itself:
						#  dataset name, datanames;
						#  done before listing
						#  actual assertion values

       #if there is more than one data set	#         +
		lea	psize,%a0		#	  |
        #if {datasets} > 8192			#	  |
		add.l	cdset,%a0		#	  |
        #else					#	  |
		add.w	cdset,%a0		#   psize[dataset]
        #endif					#	  |
		mov.l	(%a0),-(%sp)		#	  |
       #else					#	  |
		pea	PSIZE			#	  |
       #endif					#	  +

		pea	1.w

       #if there is more than one data set	#	  +
		lea	ptext,%a0		#	  |
        #if {datasets} > 8192			#	  |
		add.l	cdset,%a0		#	  |
        #else					#	  |
		add.w	cdset,%a0		#   ptext[dataset]
        #endif					#	  |
		mov.l	(%a0),-(%sp)		#	  |
       #else					#	  |
		pea	ptext			#	  |
       #endif					#	  +

		jsr	_fwrite			# fwrite(ptext[dataset],1,
						#    psize[dataset],file)
		lea	16(%sp),%sp
      #endif
     #endif

		mov.l	%a6,ver_a6		# save %a6 & %sp
		mov.l	%sp,ver_sp		# (user may stomp on them)

     #
     # Three calls are made: once each to the initialization section, 
     # time section, and verify section. Special care is taken not to 
     # mess with %sp, %a6, or %cc that the user may be depending upon.
     #

     #if there is an initialization section
		subq.w	&6,%sp			# call initialization code
		mov.w	%cc,(%sp)
		mov.l	&___Zinit,2(%sp)
		mov.l	&verify2,___ZSP		# return via ___ZSP
		rtr
	verify2:
     #endif

		subq.w	&6,%sp			# call time code
		mov.w	%cc,(%sp)
		mov.l	&___Ztime,2(%sp)
		mov.l	&verify3,___ZSP		# return via ___ZSP
		rtr

	verify3:subq.w	&6,%sp			# call verify code
		mov.w	%cc,(%sp)
		mov.l	&___Zverify,2(%sp)
		mov.l	&verify4,___ZSP		# return via ___ZSP
		rtr

	verify4:mov.l	ver_a6,%a6		# restore %a6 and %sp
		mov.l	ver_sp,%sp

     #if there is more than one data set

      #if {datasets} > 65535			# any more datasets
		subq.l	&1,-12(%a6)		#   to loop through?
      #else
		subq.w	&1,-12(%a6)
      #endif
		beq.b	verify5

      #if {datasets} > 8192			# more datasets
		addq.l	&4,cdset		#  increment dataset index
      #else					#  (this is done by 4's be-
		addq.w	&4,cdset		#  cause every table that uses
      #endif					#  this has 32-bit entries)

      #if there are dynamic parameters
		mov.l	-4(%a6),%a0	# next parameter table
		mov.l	(%a0)+,cdsetpar	# current table for current dataset
		mov.l	%a0,-4(%a6)	# save pointer to next one
      #endif

		mov.l	-8(%a6),%a0	# next assertion table
		mov.l	(%a0)+,cdsetver	# current table for current dataset
		mov.l	%a0,-8(%a6)	# save pointer to next one

		bra.b	verify

	verify5:space	0		# out of datasets
     #endif

     #if mode is assertion listing	# terminate line with linefeed
		pea	verify1
		mov.l	file,-(%sp)
		jsr	_fprintf	# fprintf(file,"\n");
		addq.w	&8,%sp
     #else
		tst.b	vererr		# was there an assert error somewhere?
		beq.b	verify6
		movq	&1,%d0		# prepare for exit(1)
		mov.l	%d0,exitval
		jmp	cleanup		# do cleanup
	verify6:space	0		# no error
     #endif
    #endif

*/

#include "fizz.h"

void Dprint_verify_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;

    if (!gAssertionCount) return;
    print(output, "\tdata\n");
    print(output, "\tlalign\t4\n");
    print(output, "ver_a6:\tspace\t4\n");
    print(output, "ver_sp:\tspace\t4\n");
    if (gMode == ASSERTION_LISTING) print(output, "verify1:\tbyte\t10,0\n");
    else print(output, "vererr:\tbyte\t0\n");
    if (gDatasetCount > 1) {
        print(output, "\tlalign\t4\n");
	print(output, "cdset:\tspace\t%c\n", gDatasetCount > 8192 ? '4' : '2');
    };
    print(output, "\ttext\n");
    if (gDatasetCount > 1) {
	if (gParameterCount) {
	    print(output, "\tlea\tdsetpar,%%a0\n");
	    print(output, "\tmov.l\t(%%a0)+,cdsetpar\n");
	    print(output, "\tmov.l\t%%a0,-4(%%a6)\n");
	};
	print(output, "\tlea\tdsetver,%%a0\n");
	print(output, "\tmov.l\t(%%a0)+,cdsetver\n");
	print(output, "\tmov.l\t%%a0,-8(%%a6)\n");
	print(output, "\tmov.%c\t&%d,-12(%%a6)\n", 
	  gDatasetCount > 65535 ? 'l' : 'w', gDatasetCount);
	print(output, "\tclr.%c\tcdset\n", gDatasetCount > 8192 ? 'l' : 'w');
    };
    print(output, "verify:\n");
    if (gMode == ASSERTION_LISTING) {
	if (gDatasetCount) {
	    print(output, "\tmove.l\tfile,-(%%sp)\n");
	    if (gDatasetCount > 1) {
		print(output, "\tlea\tpsize,%%a0\n");
		print(output, "\tadd.%c\tcdset,%%a0\n", 
		  gDatasetCount > 8192 ? 'l' : 'w');
		print(output, "\tmov.l\t(%%a0),-(%%sp)\n");
	    } else print(output, "\tpea\tPSIZE\n");
	    print(output, "\tpea\t1.w\n");
	    if (gDatasetCount > 1) {
		print(output, "\tlea\tptext,%%a0\n");
		print(output, "\tadd.%c\tcdset,%%a0\n", 
		  gDatasetCount > 8192 ? 'l' : 'w');
		print(output, "\tmov.l\t(%%a0),-(%%sp)\n");
	    } else print(output, "\tpea\tptext\n");
	    print(output, "\tjsr\t_fwrite\n");
	    print(output, "\tlea\t16(%%sp),%%sp\n");
        };
    };
    print(output, "\tmov.l\t%%a6,ver_a6\n");
    print(output, "\tmov.l\t%%sp,ver_sp\n");
    if (gfInitSection) {
	print(output, "\tsubq.w\t&6,%%sp\n");
	print(output, "\tmov.w\t%%cc,(%%sp)\n");
	print(output, "\tmov.l\t&___Zinit,2(%%sp)\n");
	print(output, "\tmov.l\t&verify2,___ZSP\n");
	print(output, "\trtr\n");
	print(output, "verify2:\n");
    };
    print(output, "\tsubq.w\t&6,%%sp\n");
    print(output, "\tmov.w\t%%cc,(%%sp)\n");
    print(output, "\tmov.l\t&___Ztime,2(%%sp)\n");
    print(output, "\tmov.l\t&verify3,___ZSP\n");
    print(output, "\trtr\n");
    print(output, "verify3:\tsubq.w\t&6,%%sp\n");
    print(output, "\tmov.w\t%%cc,(%%sp)\n");
    print(output, "\tmov.l\t&___Zverify,2(%%sp)\n");
    print(output, "\tmov.l\t&verify4,___ZSP\n");
    print(output, "\trtr\n");
    print(output, "verify4:\tmov.l\tver_a6,%%a6\n");
    print(output, "\tmov.l\tver_sp,%%sp\n");
    if (gDatasetCount > 1) {
	print(output, "\tsubq.%c\t&1,-12(%%a6)\n", 
	  gDatasetCount > 65535 ? 'l' : 'w');
	print(output, "\tbeq.b\tverify5\n");
	print(output, "\taddq.%c\t&4,cdset\n", 
	  gDatasetCount > 8192 ? 'l' : 'w');
	if (gParameterCount) {
	    print(output, "\tmov.l\t-4(%%a6),%%a0\n");
	    print(output, "\tmov.l\t(%%a0)+,cdsetpar\n");
	    print(output, "\tmov.l\t%%a0,-4(%%a6)\n");
	};
	print(output, "\tmov.l\t-8(%%a6),%%a0\n");
	print(output, "\tmov.l\t(%%a0)+,cdsetver\n");
	print(output, "\tmov.l\t%%a0,-8(%%a6)\n");
	print(output, "\tbra.w\tverify\n");
	print(output, "verify5:\tspace\t0\n");
    };
    if (gMode == ASSERTION_LISTING) {
	print(output, "\tpea\tverify1\n");
	print(output, "\tmov.l\tfile,-(%%sp)\n");
	print(output, "\tjsr\t_fprintf\n");
	print(output, "\taddq.w\t&8,%%sp\n");
    } else {
	print(output, "\ttst.b\tvererr\n");
	print(output, "\tbeq.b\tverify6\n");
	print(output, "\tmov.l\t&1,%%d0\n");
	print(output, "\tmov.l\t%%d0,exitval\n");
	print(output, "\tjmp\tcleanup\n");
	print(output, "verify6:\tspace\t0\n");
    };

    return;
}
