/*

    Dprint_comment_code()

    If there is are comments, create code to print them.

		data
	comment:
		byte	....		# bytes for comment 1
		byte	10
		byte	....		# bytes for comment 2
		byte	10
		byte	....		# bytes for comment 3
		byte	10
		    .
		    .
		    .
		byte	10		# extra linefeed

		text
		mov.l	file,-(%sp)
		pea	{length}	# length of comments + number of
					#    comment linefeeds + 1 
					#    (for extra linefeed)
		pea	1.w
		pea	comment
		jsr	_fwrite		# fwrite(comment,1,{length},file)
		lea	16(%sp),%sp

	Comments are stored internally as:

			comment 1	comment 2	comment 3
		       +---------+     +---------+     +---------+
	comments ----> |  next   |---->|  next   |---->|next = 0 |<---+
		       +---------+     +---------+     +---------+    |
		       |  line   |     |  line   |     |  line   |    |
		       +---+-----+     +---+-----+     +---+-----+    |
			   |               |               |          |
                           |               |               |     comment_tail
                           |               |               |
			   V		   V               V
		   "comment 1 text"  "comment 2 text"  "comment 3 text"
*/

#include "fizz.h"

static struct comment { 
    struct comment *next; 
    char *line; 
} *comments = (struct comment *) 0, 
  *comment_tail = (struct comment *) 0;

static char *comment_code1[] = {
	"\tdata",
	"comment:",
};

static char *comment_code2[] = {
	"\tbyte\t10",
	"\ttext",
	"\tmov.l\tfile,-(%sp)",
};

static char *comment_code3[] = {
	"\tpea\t1.w",
	"\tpea\tcomment",
	"\tjsr\t_fwrite",
	"\tlea\t16(%sp),%sp",
};

void Dprint_comment_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;
    unsigned long comment_count, comment_length;
    struct comment *comment_ptr;

    if (comments != (struct comment *) 0) {
        for (count = 0; count < sizeof(comment_code1)/sizeof(char *); count++)
	    print(output, "%s\n", comment_code1[count]);
        comment_ptr = comments;
	comment_count = 0;
	comment_length = 0;
	do {
	    comment_count++;
	    comment_length += strlen(comment_ptr->line);
	    print_byte_info(comment_ptr->line);
	    print(output, "\tbyte\t10\n");
	    comment_ptr = comment_ptr->next;
	} while (comment_ptr != (struct comment *) 0);
        for (count = 0; count < sizeof(comment_code2)/sizeof(char *); count++)
	    print(output, "%s\n", comment_code2[count]);
	print(output, "\tpea\t%d\n", comment_length + comment_count + 1);
        for (count = 0; count < sizeof(comment_code3)/sizeof(char *); count++)
	    print(output, "%s\n", comment_code3[count]);
    };
    return;
}

/*

    instr_comment()

    Process a comment instruction.

*/

void instr_comment(comment_string)
char *comment_string;
{
    unsigned long size;
    register int c;
    register char *ptr_start, *ptr_end;
    struct comment *comment_ptr;

    /* Comments are only allowed in the fizz initialization section */
    if (gSection != FIZZ_INIT) {
	error_locate(27);
	return;
    };

    /* Search for the beginning of the comment */
    ptr_start = comment_string;
    while ((c = *ptr_start++) && isspace(c));
    --ptr_start;

    if (c) { /* null comment? */
	ptr_end = &comment_string[strlen(comment_string)];
	while ((c = *--ptr_end) && isspace(c));	/* remove trailing whitespace */
	size = ptr_end - ptr_start + 1;
    } else size = 0;

    CREATE_STRUCT(comment_ptr, comment); /* for new comment */

    /* Get space for storing the comment */
    if ((comment_ptr->line = malloc(size + 1)) == (char *) 0) {
	free(comment_ptr);
	error(2, (char *) 0);
    };

    /* Save the comment */
    if (size) (void) strncpy(comment_ptr->line, ptr_start, size);
    comment_ptr->line[size] = '\0';

    /* Link new comment in with other comments */
    comment_ptr->next = (struct comment *) 0;
    if (comment_tail != (struct comment *) 0) comment_tail->next = comment_ptr;
    else comments = comment_ptr;
    comment_tail = comment_ptr;

    return;
}
