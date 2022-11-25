/*

    Dprint_time_code()

    Print code to execute timing loops with full timing and overhead timing.

    NOTE:
	{iterations} = number of iterations
	{loops} = {iterations} / sum of dataset counts
	{max_count} = maximum dataset count

	For the general case, the algorithm is:

	    for (I = 0; I < {loops}; I++) {
		for (J = 0; J < number of datasets; J++) {
		    for (K = 0; K < count for dataset J; K++) {
			INITIALIZE
			TIME (or OVERHEAD)
		    };
		};
	    };




		data
		lalign	4
	time_a6:    space 4	# for saving a6 & sp in case user
	time_sp:    space 4	#	stomps on them
	time_start: space 8	# beginning time
	time_end:   space 8	# ending time
	time_target:space 4	# routine to time
	time_flag:  space 1	# time or overhead loop?

		text

		clr.b	time_flag
		mov.l	&___Ztime,time_target
	time1:

    #if there is more than one data set
     #if {loops} > 65535			# outer loop
		mov.l	&{loops},-24(%a6)	# number of overall loops
     #else
		mov.w	&{loops},-24(%a6)
     #endif
    #endif

		pea	stat5		# start timing
		pea	time_start
		jsr	_gettimeofday
		addq.w	&8,%sp

    #if there is more than one data set
	time2:

     #if there are dynamic parameters
		lea	dsetpar,%a0		# parameters for this dataset
		mov.l	(%a0)+,cdsetpar		# save for quick reference
		mov.l	%a0,-4(%a6)		# save pointer to next table
     #endif

     #if {datasets} > 65535			# next loop
		mov.l	&{datasets},-12(%a6)	# for each dataset...
     #else
		mov.w	&{datasets},-12(%a6)
     #endif

		lea	count,%a0
     #if {max_count} > 65535		# get count associated with dataset
		mov.l	(%a0)+,-20(%a6)
     #else
		mov.w	(%a0)+,-20(%a6)
     #endif
		mov.l	%a0,-16(%a6)		# save ptr to count for 
						#    next dataset

    #else	(zero or one dataset)

     #if number of iterations > 65535		# just loop through the whole
		mov.l	&{iterations},-20(%a6)	#    thing the specified number
     #else					#    iterations
		mov.w	&{iterations},-20(%a6)
     #endif

    #endif

		bra.w	time3			# to guarantee code alignment
		lalign	4

	time3:	mov.l	%a6,time_a6		# save a6 & sp in case 
		mov.l	%sp,time_sp		#     user destroys them

    #if there is an initialization section
		subq.w	&6,%sp			# we are very careful not to 
		mov.w	%cc,(%sp)		#    mess with any registers or 
		mov.l	&___Zinit,2(%sp)	#    condition codes that the 
		mov.l	&time4,___ZSP		#    user has set up
		rtr
	time4:	
    #endif

		subq.w	&6,%sp
		mov.w	%cc,(%sp)
		mov.l	time_target,2(%sp)	# call ___Zinit or ___Zovhd
		mov.l	&time5,___ZSP
		rtr

	time5:	mov.l	time_a6,%a6		# restore %a6 and %sp
		mov.l	time_sp,%sp

    #if there is more than one data set	 (depending on the loop structure...)

     #if {max_count} > 65535
		subq.l	&1,-20(%a6)		# one less count for dataset
     #else
		subq.w	&1,-20(%a6)
     #endif

    #else (zero or one dataset)

     #if number of iterations > 65535		# one less iteration
		subq.l	&1,-20(%a6)
     #else
		subq.w	&1,-20(%a6)
     #endif

    #endif
		bne.b	time3			# loop again (inner loop)

    #if there is more than one data set # (inner loop exhausted)

     #if {datasets} > 65535
		subq.l	&1,-12(%a6)		# decrement datasets remaining
     #else
		subq.w	&1,-12(%a6)
     #endif
		beq.b	time6			# any datasets left?

     #if there are dynamic parameters		# datasets left
		mov.l	-4(%a6),%a0		# next dataname parameter table
		mov.l	(%a0)+,cdsetpar		# for quick reference
		mov.l	%a0,-4(%a6)		# save pointer to next one
     #endif
  
		mov.l	-16(%a6),%a0		# get count for next dataset
     #if {max_count} > 65535
		mov.l	(%a0)+,-20(%a6)
     #else
		mov.w	(%a0)+,-20(%a6)
     #endif
		mov.l	%a0,-16(%a6)		# pointer to next count

		bra.b	time3			# loop again


	time6:
     #if {loops} > 65535
		subq.l	&1,-24(%a6)		# counter for outer loop
     #else
		subq.w	&1,-24(%a6)
     #endif
		bne.w	time2			# any work left?
    #endif

		pea	stat5			# stop timing
		pea	time_end
		jsr	_gettimeofday
		addq.w	&8,%sp

		tst.b	time_flag
		bne.b	time7
		st	time_flag
		mov.l	time_start,stat2
		mov.l	time_start+4,stat2+4
		mov.l	time_end,stat1
		mov.l	time_end+4,stat1+4
		mov.l	&___Zovhd,time_target
		bra.w	time1
	time7:	mov.l	time_start,stat4
		mov.l	time_start+4,stat4+4
		mov.l	time_end,stat3
		mov.l	time_end+4,stat3+4

*/

#include "fizz.h"

void Dprint_time_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    unsigned long loops, max_count;

    if (gDatasetCount > 1) {
        loops = gIterations / sum_dataset_count();
        max_count = max_dataset_count();
    };

    print(output, "\tdata\n");
    print(output, "\tlalign\t4\n");
    print(output, "time_a6:\tspace\t4\n");
    print(output, "time_sp:\tspace\t4\n");
    print(output, "time_start:\tspace\t8\n");
    print(output, "time_end:\tspace\t8\n");
    print(output, "time_target:\tspace\t4\n");
    print(output, "time_flag:\tspace\t1\n");
    print(output, "\ttext\n");
    print(output, "\tclr.b\ttime_flag\n");
    print(output, "\tmov.l\t&___Ztime,time_target\n");
    print(output, "time1:\n");

    if (gDatasetCount > 1)
	print(output, "\tmov.%c\t&%u,-24(%%a6)\n",
	  loops > 65535 ? 'l' : 'w', loops);

    print(output, "\tpea\tstat5\n");
    print(output, "\tpea\ttime_start\n");
    print(output, "\tjsr\t_gettimeofday\n");
    print(output, "\taddq.w\t&8,%%sp\n");

    if (gDatasetCount > 1) {
	print(output, "time2:\n");
	if (gParameterCount) {
	    print(output, "\tlea\tdsetpar,%%a0\n");
	    print(output, "\tmov.l\t(%%a0)+,cdsetpar\n");
	    print(output, "\tmov.l\t%%a0,-4(%%a6)\n");
	};
	print(output, "\tmov.%c\t&%u,-12(%%a6)\n", 
	  gDatasetCount > 65535 ? 'l' : 'w', gDatasetCount);
	print(output, "\tlea\tcount,%%a0\n");
	print(output, "\tmov.%c\t(%%a0)+,-20(%%a6)\n", 
	  max_count > 65535 ? 'l' : 'w');
	print(output, "\tmov.l\t%%a0,-16(%%a6)\n");
    } else print(output, "\tmov.%c\t&%u,-20(%%a6)\n", 
	  gIterations > 65535 ? 'l' : 'w', gIterations);

    print(output, "\tbra.w\ttime3\n");
    print(output, "\tlalign\t4\n");
    print(output, "time3:\tmov.l\t%%a6,time_a6\n");
    print(output, "\tmov.l\t%%sp,time_sp\n");

    if (gfInitSection) {
	print(output, "\tsubq.w\t&6,%%sp\n");
	print(output, "\tmov.w\t%%cc,(%%sp)\n");
	print(output, "\tmov.l\t&___Zinit,2(%%sp)\n");
	print(output, "\tmov.l\t&time4,___ZSP\n");
	print(output, "\trtr\n");
	print(output, "time4:\n");
    };

    print(output, "\tsubq.w\t&6,%%sp\n");
    print(output, "\tmov.w\t%%cc,(%%sp)\n");
    print(output, "\tmov.l\ttime_target,2(%%sp)\n");
    print(output, "\tmov.l\t&time5,___ZSP\n");
    print(output, "\trtr\n");
    print(output, "time5:\tmov.l\ttime_a6,%%a6\n");
    print(output, "\tmov.l\ttime_sp,%%sp\n");

    if (gDatasetCount > 1) print(output, "\tsubq.%c\t&1,-20(%%a6)\n", 
      max_count > 65535 ? 'l' : 'w');
    else print(output, "\tsubq.%c\t&1,-20(%%a6)\n", 
      gIterations > 65535 ? 'l' : 'w');
    print(output, "\tbne.b\ttime3\n");

    if (gDatasetCount > 1) {
        print(output, "\tsubq.%c\t&1,-12(%%a6)\n", 
          gDatasetCount > 65535 ? 'l' : 'w');
	print(output, "\tbeq.b\ttime6\n");
	if (gParameterCount) {
	    print(output, "\tmov.l\t-4(%%a6),%%a0\n");
	    print(output, "\tmov.l\t(%%a0)+,cdsetpar\n");
	    print(output, "\tmov.l\t%%a0,-4(%%a6)\n");
	};
        print(output, "\tmov.l\t-16(%%a6),%%a0\n");
        print(output, "\tmov.%c\t(%%a0)+,-20(%%a6)\n",
          max_count > 65535 ? 'l' : 'w');
        print(output, "\tmov.l\t%%a0,-16(%%a6)\n");
        print(output, "\tbra.b\ttime3\n");
        print(output, "time6:\n");
        print(output, "\tsubq.%c\t&1,-24(%%a6)\n",
          loops > 65535 ? 'l' : 'w');
        print(output, "\tbne.w\ttime2\n");
    };

    print(output, "\tpea\tstat5\n");
    print(output, "\tpea\ttime_end\n");
    print(output, "\tjsr\t_gettimeofday\n");
    print(output, "\taddq.w\t&8,%%sp\n");

    print(output, "\ttst.b\ttime_flag\n");
    print(output, "\tbne.b\ttime7\n");
    print(output, "\tst\ttime_flag\n");
    print(output, "\tmov.l\ttime_start,stat2\n");
    print(output, "\tmov.l\ttime_start+4,stat2+4\n");
    print(output, "\tmov.l\ttime_end,stat1\n");
    print(output, "\tmov.l\ttime_end+4,stat1+4\n");
    print(output, "\tmov.l\t&___Zovhd,time_target\n");
    print(output, "\tbra.w\ttime1\n");
    print(output, "time7:\tmov.l\ttime_start,stat4\n");
    print(output, "\tmov.l\ttime_start+4,stat4+4\n");
    print(output, "\tmov.l\ttime_end,stat3\n");
    print(output, "\tmov.l\ttime_end+4,stat3+4\n");

    return;
}
