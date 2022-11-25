/*

    Dprint_title_code()

    If there is a title, create code to print it.

		data
	title:
		byte	....		# title bytes
		byte	10,10		# extra linefeed

		text
		mov.l	file,-(%sp)
		pea	{size}		# length of title + 2
		pea	1.w
		pea	title
		jsr	_fwrite		# fwrite(title,1,{size},file)
		lea	16(%sp),%sp

*/

#include "fizz.h"

static char *title;			/* title string */

static char *title_code1[] = {
	"\tdata",
	"title:",
};

static char *title_code2[] = {
	"\tbyte\t10,10",
	"\ttext",
	"\tmov.l\tfile,-(%sp)",
};

static char *title_code3[] = {
	"\tpea\t1.w",
	"\tpea\ttitle",
	"\tjsr\t_fwrite",
	"\tlea\t16(%sp),%sp",
};

void Dprint_title_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (title != (char *) 0) {
        for (count = 0; count < sizeof(title_code1)/sizeof(char *); count++)
	    print(output, "%s\n", title_code1[count]);
	print_byte_info(title);		/* title string */
        for (count = 0; count < sizeof(title_code2)/sizeof(char *); count++)
	    print(output, "%s\n", title_code2[count]);
	print(output, "\tpea\t%d\n", strlen(title) + 2);	/* {size} */
        for (count = 0; count < sizeof(title_code3)/sizeof(char *); count++)
	    print(output, "%s\n", title_code3[count]);
    };
    return;
}

/*

    store_title()

    Store the title.

*/

void store_title(source, title_string)
short source;
char *title_string;
{
    static BOOLEAN cmdline = FALSE;
    static BOOLEAN cmdline_error = FALSE;
    static BOOLEAN infile = FALSE;
    static BOOLEAN infile_error = FALSE;
    BOOLEAN store;
    register int c;
    register char *ptr_start, *ptr_end;
    unsigned long size;

    store = FALSE;

    if (source == CMDLINE) {
	if (cmdline) {
	    if(!cmdline_error) {
	        error(41, (char *) 0);
	        cmdline_error = TRUE;
	    };
	} else {
	    store = TRUE;
	    cmdline = TRUE;
        };
    } else {
	if (infile) {
	    if (!infile_error) {
	        error_locate(42);
	        infile_error = TRUE;
	    };
	} else {
	    if (!cmdline) store = TRUE;
	    infile = TRUE;
        };
    };

    if (store) {
	ptr_start = title_string;
	while ((c = *ptr_start++) && isspace(c));
	--ptr_start;
	if (c) {
	    ptr_end = &title_string[strlen(title_string)];
	    while ((c = *--ptr_end) && isspace(c));
	    size = ptr_end - ptr_start + 1;
	} else size = 0;
	CREATE_STRING(title, size + 1);
	if (size) (void) strncpy(title, ptr_start, size);
	title[size] = '\0';
    };

    return;
}
