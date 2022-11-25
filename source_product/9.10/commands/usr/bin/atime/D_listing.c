/*

    Dprint_listing_code()

    Create code to print a listing.

    #if nolist is turned off
		data
	infile:
		byte	....		# file contents
		byte	....
		byte	....
		byte	....
		byte	....
		byte	10

		text
		mov.l	file,-(%sp)
		pea	{length}	# file length (linefeeds and everything)
		pea	1.w
		pea	infile
		jsr	_fwrite		# fwrite(infile,1,{length},file);
		lea	16(%sp),%sp
    #endif

    The input file is opened and read a line at a time, checking for
    include files. These lines are converted to byte instructions and
    dumped to the assembly file.

*/

#include "fizz.h"

void Dprint_listing_code()
{
    char *tk_start, *tk_end;
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    unsigned long length;

    if (!gfNolist) {
	print(output, "\tdata\n");
	print(output, "infile:\n");
	length = 0;
        open_input_file(gInputFile, FALSE);	/* open input file */
        while(read_line()) {			/* while more file */
	    /* search for "include" */
	    get_next_token(gBuffer, &tk_start, &tk_end); 
	    /* ignore label */
	    if ((tk_start != (char *) 0) && (*(tk_end - 1) == ':'))
		get_next_token(tk_end, &tk_start, &tk_end);
	    if ((tk_start != (char *) 0) && (tk_end - tk_start == 7) &&
	      !strncmp(tk_start, "include", 7)) {
		instr_include(tk_end, FALSE);
		continue;
	    };
	    print_byte_info(gBuffer);		/* dump line */
	    print(output, "\tbyte\t10\n");	/* terminate with linefeed */
	    length += strlen(gBuffer) + 1;	/* update length */
	};
	close_input_files();			/* finish it off */
	print(output, "\tbyte\t10\n");		/* one more linefeed */
	length++;
	print(output, "\ttext\n");
	print(output, "\tmov.l\tfile,-(%%sp)\n");
	print(output, "\tpea\t%u\n", length);
	print(output, "\tpea\t1.w\n");
	print(output, "\tpea\tinfile\n");
	print(output, "\tjsr\t_fwrite\n");
	print(output, "\tlea\t16(%%sp),%%sp\n");
    };
}
