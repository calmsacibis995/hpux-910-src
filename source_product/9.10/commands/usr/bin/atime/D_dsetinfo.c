/*

    Dprint_dataset_info_table()

    Print tables of information for assertion listing. An example of a
    string in such a table would be:

	dataset: set1
		 $number	       0x80
		 $age		         25
		 $prefix	         12

    The table is effectively of the type "char *ptext[]" with the string
    sizes in "unsigned long psize[]". (Except when there is only one data
    set when the string is "char *ptext" and its length, "PSIZE".)


	#if mode is assertion listing
	 #if there are data sets
		data
		lalign	4
	ptext:
	  #if there is more than one data set
		long	ptext0		# initialize char *ptext[];
		long	ptext1
		long	ptext2
		long	ptext3
		long	ptext4
		      .
		      .
		      .
	ptext0:
		byte	10		# extra linefeed before first one
		{text for data set 0}
	ptext1:
		{text for data set 1}
	ptext2:
		{text for data set 2}
	ptext3:
		{text for data set 3}
	ptext4:
		{text for data set 4}
		      .
		      .
		      .
	  #else
		byte	10		# extra linefeed
		{text for data set}
	  #endif
	  #if there is more than one data set
		lalign	4
	psize:				# number of bytes in each string
		long	{psize0}
		long	{psize1}
		long	{psize2}
		long	{psize3}
		long	{psize4}
		      .
		      .
		      .
	  #else
		set	PSIZE,{psize}	# passed via global constant
	  #endif
	 #endif
	#endif
*/

#include "fizz.h"

void Dprint_dataset_info_table()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register unsigned long i, j;
    unsigned long length;
    char *ptr;
    unsigned long *psize;		/* table of string lengths */
    unsigned long pwidth;		/* length of longest name */
    char *blanks = "                                                  ";
		/* for padding */

    if ((gMode == ASSERTION_LISTING) && (gDatasetCount)) {
	pwidth = printwidth();		/* get name width */
	print(output, "\tdata\n");
	print(output, "\tlalign\t4\n");
	print(output, "ptext:\n");
        if ((psize = (unsigned long *) malloc(gDatasetCount * 
	  sizeof(unsigned long))) == (unsigned long *) 0) 
	    error(2, (char *) 0);	/* allocate string length table */
	if (gDatasetCount > 1)
	    for (i = 0; i < gDatasetCount; i++)
		print(output, "\tlong\tptext%u\n", i);
	for (i = 0; i < gDatasetCount; i++) {
	    if (gDatasetCount > 1) print(output, "ptext%u:\n", i);
	    if (!i) print(output, "\tbyte\t10\n");	/* extra linefeed */
	    print(output, "\tbyte\t9,\"dataset: \"\n");
	    ptr = get_dataset_name(i);	/* dataset name */
	    print_byte_info(ptr);
	    print(output, "\tbyte\t10\n");	/* linefeed after dataset */
	    psize[i] = strlen(ptr) + 11 + !i;
	    for (j = 0; j < gDatanameCount; j++) {
		print(output, "\tbyte\t9,9\n");
		ptr = get_dataname(j);
		print_byte_info(ptr);	/* dataname */
		length = pwidth - strlen(ptr);
		while (length > 50) {	/* blank padding so all names 
					   are same size */
		    print(output, "\tbyte\t\"%s\"\n", blanks);
		    length -= 50;
		};
		if (length)
		    print(output, "\tbyte\t\"%*.s\"\n", length, blanks);
		psize[i] += pwidth + 2;
		ptr = get_datum(j, i);	/* dataset value */
		length = strlen(ptr);
		if (length >= 14) {
		    print(output, "\tbyte\t\" \"\n");
		    print_byte_info(ptr);
		    print(output, "\tbyte\t10\n");
		    psize[i] += length + 2;
		} else {
		    print(output, "\tbyte\t\"%*.s\"\n",14 - length, blanks);
		    print_byte_info(ptr);
		    print(output, "\tbyte\t10\n");
		    psize[i] += 15;
		};
	    };
	};
	print(output, "\tlalign\t4\n");
	if (gDatasetCount > 1) {	/* print string length table */
	    print(output, "psize:\n");
	    for (i = 0; i < gDatasetCount; i++)
	        print(output, "\tlong\t%u\n", psize[i]);
	} else print(output, "\tset\tPSIZE,%u\n",psize[0]);
	free(psize);
    };
    return;
}
