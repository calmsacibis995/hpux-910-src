/*

    process_assert_file()

    Read the assertion file and store all pertinent information.

    datasets--->+----------+
	   +----|   next   |   +--->"dataset name 1"
	   |    +----------+   |
	   |    |   name   |---+
	   |    +----------+    +----------+    +----------+
	   |    |assertion_|--->|   next   |--->| next = 0 |
	   |    |   list   |    +----------+    +----------+
	   |    +----------+    |   name   |-+  |   name   |-+
	   |                    +----------+ |  +----------+ |
	   |                    |   value  | |  |   value  | |
	   |			+----------+ |  +----------+ |
	   |                    |   size   | |  |   size   | |
	   |			+----------+ |  +----------+ |
	   |				     V               V
	   |				"assertion name 1" "assertion name 2"
	   |
           +--->+----------+
	        | next = 0 |   +--->"dataset name 2"
	        +----------+   |
	        |   name   |---+
	        +----------+    +----------+    +----------+
	        |assertion_|--->|   next   |--->| next = 0 |
	        |   list   |    +----------+    +----------+
	        +----------+    |   name   |-+  |   name   |-+
	                        +----------+ |  +----------+ |
	                        |   value  | |  |   value  | |
	    			+----------+ |  +----------+ |
	                        |   size   | |  |   size   | |
	    			+----------+ |  +----------+ |
	    				     V               V
	    				"assertion name 1" "assertion name 2"
*/

#include "fizz.h"

struct assertion_type {
    struct assertion_type *next;
    char *name;
    int value;
    short size;
};

static struct dataset_type {
    struct dataset_type *next;
    char *name;
    struct assertion_type *assertion_list;
} *datasets = (struct dataset_type *) 0;

void process_assert_file()
{
    register int c;
    register char *ptr;
    char *tk_start, *tk_end, *name_start, *name_end;
    int value;
    BOOLEAN sign, error_flag, valid_dataset;
    struct dataset_type *current_dataset;
    struct assertion_type *current_assertion;

    valid_dataset = TRUE;
    open_input_file(gAssertFile, FALSE);

    while (read_line()) {			/* get next line */
	ptr = gBuffer;
	if (!isspace(c = *ptr++)) continue;	/* skip lines that don't
						   start with white space */
	while (isspace(c = *ptr++));		/* search for token */
	if (c == '$') continue;			/* skip datanames */
	if (!isalpha(c)) {			/* must start with alpha char */
	    error_locate(23);
            continue;
	};
	name_start = ptr - 1;
	if (!strncmp(name_start, "dataset:", 8)) {	/* dataset? */
	    get_next_token(name_start + 8, &tk_start, &tk_end);
						/* get dataset name */
	    if (!(ptr = tk_start)) {		/* name missing? */
		valid_dataset = FALSE;
		error_locate(33);
		continue;
	    };

	    /* check for valid dataset name */
	    if (!isalpha(*ptr++)) {
		valid_dataset = FALSE;
	        error_locate(24);
                continue;
	    };
	    while (ptr < tk_end)
		if (!(isalnum(c = *ptr++) || (c == '_'))) {
		    valid_dataset = FALSE;
	            error_locate(24);
                    break;
	        };
	    if (ptr < tk_end) continue;

	    /* check for duplicate dataset name */
	    current_dataset = datasets;
	    while (current_dataset != (struct dataset_type *) 0) {
		if ((strlen(current_dataset->name) == (tk_end - tk_start)) &&
		  (!strncmp(current_dataset->name, tk_start, 
		  tk_end - tk_start))) {
		    error_locate(121);
		    valid_dataset = FALSE;
		};
		current_dataset = current_dataset->next;
	    };

	    /* create storage structures and save information */
	    if (current_dataset != (struct dataset_type *) 0) continue;
	    CREATE_STRUCT(current_dataset, dataset_type);
	    CREATE_STRING(current_dataset->name, tk_end - tk_start + 1);
	    (void) strncpy(current_dataset->name, tk_start, tk_end - tk_start);
	    current_dataset->name[tk_end - tk_start] = '\0';
	    current_dataset->next = datasets;
	    current_dataset->assertion_list = (struct assertion_type *) 0;
	    datasets = current_dataset;
	    valid_dataset = TRUE;
	    continue;
	};

	while (isalnum(c = *ptr++) || (c == '_'));  /* scan assertion name */
	if (!c) {			/* missing data? */
	    error_locate(30);
            continue;
	};
	if (!isspace(c)) {		/* bad character in name? */
	    error_locate(23);
            continue;
	};
	name_end = ptr - 1;
	get_next_token(ptr, &tk_start, &tk_end);	/* get value */
	if (!(ptr = tk_start)) {	/* no value? */
	    error_locate(30);
            continue;
	};

	/* process value */
	sign = FALSE;
	if (*ptr == '-') {
	    sign = TRUE;
	    ptr++;
	};
	if (!isdigit(c = *ptr++)) {
	    error_locate(22);
            continue;
	};
	value = c - '0';
	error_flag = FALSE;
	while (isdigit(c = *ptr++)) {		/* digit? */
	    if ((value > 0x0CCCCCCC) || ((value == 0x0CCCCCCC) /* overflow? */
	      && (c > (sign ?  '8' : '7')))) {
	        error_flag = TRUE;
	        error_locate(43);
	        break;
	    };
	    value += value;	/* value = value * 10 + digit */
	    value += value << 2;
	    value += c - '0';
	};
	if (error_flag) continue;	/* overflow? */
	if (c != '.') {			/* period for suffix? */
	    error_locate(34);
	    continue;
	};
	if (sign) value = -value;

	/* check for valid suffix */
	c = *ptr++;
	if (!((ptr == tk_end) && ((c == 'b') || (c == 'w') || (c == 'l')))) {
	    error_locate(25);
	    continue;
	};

	/* check for overflow based on suffix */
	if (((c == 'b') && ((value > 127) || (value < -128))) ||
	  ((c == 'w') && ((value > 65535) || (value < -65536)))) {
	    error_locate(43);
	    continue;
	};

	if (datasets != (struct dataset_type *) 0) {
	    /* check for duplicate assertion name */
	    current_assertion = datasets->assertion_list;
	    while (current_assertion != (struct assertion_type *) 0) {
	        if ((strlen(current_assertion->name) == (name_end - name_start))
	          && (!strncmp(current_assertion->name, name_start, 
	          name_end - name_start))) {
		    error_locate(123);
		    break;
	        };
	    };
	    if (current_assertion != (struct assertion_type *) 0) continue;
	};

	if (!valid_dataset) continue;

	/* create first dataset structure */
	if (datasets == (struct dataset_type *) 0) {
	    CREATE_STRUCT(current_dataset, dataset_type);
	    current_dataset->next = (struct dataset_type *) 0;
	    current_dataset->name = (char *) 0;
	    current_dataset->assertion_list = (struct assertion_type *) 0;
	    datasets = current_dataset;
	};

	/* add assertion data to the dataset information */
	CREATE_STRUCT(current_assertion, assertion_type);
	CREATE_STRING(current_assertion->name, name_end - name_start + 1);
        (void) strncpy(current_assertion->name, name_start, 
	  name_end - name_start);
        current_assertion->name[name_end - name_start] = '\0';
        current_assertion->value = value;
        current_assertion->size = c;
        current_assertion->next = datasets->assertion_list;
	datasets->assertion_list = current_assertion;
    };

    close_input_files();

    return;
}

/*

    void Dprint_dsetver_table()

    Create a table with assertion information.  In general, for each
    dataset there is an entry in the table "dsetver".  Each of these
    points to a separate table with assertion information.  There is one
    entry in each of the latter tables for each "assert.[bwl]" in the
    input file.  This entry has two items - the size and the value.  The
    size may be 'b', 'w', or 'l' for byte, word, and long respectively.
    It may also be -1 if the value was not found in an assertion file,
    or 0 if this particular assert instruction has already been
    executed.  The value item has been sign extended to long as
    necessary.  For performance improvement, "cdsetver" contains an
    entry from the "dsetver" table for the dataset currently under
    consideration.

    #if there are assertions

		data
		lalign	4

     #if there is more than one data set
	cdsetver:
		space	4
	dsetver:
		long	assert0
		long	assert1
		long	assert2
		long	assert3
		long	assert4
		long	assert5
		long	assert6
		      .
		      .
		      .
	assert0:
		short	{size}
		long	{value}
		short	{size}
		long	{value}
		short	{size}
		long	{value}
		short	{size}
		long	{value}
		short	{size}
		long	{value}
		      .
		      .
		      .
	assert1:
		short	{size}
		long	{value}
		short	{size}
		long	{value}
		short	{size}
		long	{value}
		      .
		      .
		      .
     #else
	assert:
		short	{size}
		long	{value}
		short	{size}
		long	{value}
		short	{size}
		long	{value}
		      .
		      .
		      .
     #endif
    #endif

*/

void Dprint_dsetver_table()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    char *dataset_name, *assertion_name;
    unsigned long i,j;
    struct dataset_type *current_dataset;
    struct assertion_type *current_assertion;

    if (gAssertionCount) {
	print(output, "\tdata\n");
	print(output, "\tlalign\t4\n");
	if (gDatasetCount > 1) {
	    print(output, "cdsetver:\n");
	    print(output, "\tspace\t4\n");
	    print(output, "dsetver:\n");
	    for (i = 0; i < gDatasetCount; i++)
		print(output, "\tlong\tassert%d\n",i);
	    for (i = 0; i < gDatasetCount; i++) {
		print(output, "assert%d:\n", i);
		dataset_name = get_dataset_name(i);
		current_dataset = datasets;
		while (current_dataset != (struct dataset_type *) 0) {
		    if (!strcmp(current_dataset->name, dataset_name)) 
			break;
		    current_dataset = current_dataset->next;
		};
		if (current_dataset == (struct dataset_type *) 0) {
		    for (j = 0; j < gAssertionCount; j++) {
			print(output, "\tshort\t-1\n");
			print(output, "\tlong\t0\n");
		    };
		} else {
		    for (j = 0; j < gAssertionCount; j++) {
		        assertion_name = get_assertion_name(j);
			current_assertion = current_dataset->assertion_list;
			while (current_assertion != 
			  (struct assertion_type *) 0) {
			    if (!strcmp(current_assertion->name, 
			      assertion_name))
			        break;
			    current_assertion = current_assertion->next;
			};
			if (current_assertion == (struct assertion_type *) 0) {
			    print(output, "\tshort\t-1\n");
			    print(output, "\tlong\t0\n");
			} else {
			    print(output, "\tshort\t%d\n", 
			      current_assertion->size);
			    print(output, "\tlong\t%d\n", 
			      current_assertion->value);
			};
		    };
	        };
	    };
	} else {
	    print(output, "assert:\n");
	    if ((datasets != (struct dataset_type *) 0)) {
		current_dataset = datasets;
		/*
		    We have to be careful here because we may want to
		    match up an unnamed dataset from the assertion file
		    with the assertions from a file having no explicit
		    datasets.
		*/
		if (gDatasetCount) {
		    dataset_name = get_dataset_name(0);
		    while (current_dataset != (struct dataset_type *) 0) {
			if (!strcmp(dataset_name, current_dataset->name)) break;
			current_dataset = current_dataset->next;
		    };
		} else {
		    while (current_dataset != (struct dataset_type *) 0) {
			if (current_dataset->name == (char *) 0) break;
			current_dataset = current_dataset->next;
		    };
		};
		if (current_dataset == (struct dataset_type *) 0) {
	            for (j = 0; j < gAssertionCount; j++) {
		        print(output, "\tshort\t-1\n");
		        print(output, "\tlong\t0\n");
	            };
		} else {
		    for (j = 0; j < gAssertionCount; j++) { 
		        assertion_name = get_assertion_name(j);
		        current_assertion = current_dataset->assertion_list;
		        while (current_assertion != 
			  (struct assertion_type *) 0) {
			    if (!strcmp(current_assertion->name, 
			      assertion_name)) break;
			    current_assertion = current_assertion->next;
		        };
		        if (current_assertion == (struct assertion_type *) 0) {
			    print(output, "\tshort\t-1\n");
			    print(output, "\tlong\t0\n");
		        } else {
			    print(output, "\tshort\t%d\n", 
			      current_assertion->size);
			    print(output, "\tlong\t%d\n", 
			      current_assertion->value);
		        };
		    };
		};
	    } else {
	        for (j = 0; j < gAssertionCount; j++) {
		    print(output, "\tshort\t-1\n");
		    print(output, "\tlong\t0\n");
	        };
	    };
	};
    };

    return;
}
