/*

    This file contains several routines associated with dataname and
    dataset management.

		 +----------+    +----------+    +----------+
    datanames--->|   next   |--->|   next   |--->| next = 0 |
		 +----------+    +----------+    +----------+
		 |   name   |    |   name   |    |   name   |
		 +----+-----+    +----+-----+    +----+-----+
		      |		      |               |
		      V		      V		      V
		  "dataname 1"    "dataname 2"    "dataname 3"

			       ---*---

     datasets--->+----------+
           +-----+   next   |
           |     +----------+    +----------+    +----------+
           |     |   data   |--->|   next   |--->| next = 0 |
           |     +----------+    +----------+    +----------+
           |  +--+   name   |    |   datum  |    |   datum  |
           |  |  +----------+    +-----+----+    +-----+----+
	   |  |  |   count  |          |               |
           |  |  +----------+          V               V
	   |  |                    "datum name"    "datum name"
           |  +--->"dataset name 1"
	   |
           +---->+----------+<---datasets_tail
                 | next = 0 |
                 +----------+    +----------+    +----------+
                 |   data   |--->|   next   |--->| next = 0 |
                 +----------+    +----------+    +----------+
              +--+   name   |    |   datum  |    |   datum  |
              |  +----------+    +-----+----+    +-----+----+
	      |  |   count  |          |               |
              |  +----------+          V               V
	      |                    "datum name"    "datum name"
              +--->"dataset name 2"

			       ---*---

							datalist_tail
								 |
		 +----------+    +----------+    +----------+<---+
     datalist--->|   next   |--->|   next   |--->| next = 0 |
		 +----------+    +----------+    +----------+
		 |   name   |    |   name   |    |   name   |
		 +----+-----+    +----+-----+    +----+-----+
		      |		      |               |
		      |		      |		      V
		      |               V   "text for dataset instruction 2"
		      V    "text for dataset instruction 1"
		"text for dataname instruction"

			       ---*---
    dataset_names
	 |
         +------>+----------+    +----------+    +----------+
                 |   next   |--->|   next   |--->| next = 0 |
		 +----------+    +----------+    +----------+
		 |   name   |    |   name   |    |   name   |
		 +----+-----+    +----+-----+    +----+-----+
		      |		      |               |
		      V		      V		      V
		  "dataset name 1" "dataset name 2" "dataset name 3"

	(Every dataset instruction encountered will have its name put on
	 the dataset_names list.  However, only those datasets
	 specifically requested (as in a -p or -l option) will be added
	 to the datasets list.  The dataset_names list is used solely to
	 check for duplicate names.)

*/

#include "fizz.h"

static struct dataname_type {
    struct dataname_type *next;
    char *name;
} *datanames = (struct dataname_type *) 0; 

struct datum_type {
    struct datum_type *next;
    char *datum;
};

static struct dataset_type {
    struct dataset_type *next;
    struct datum_type *data;
    char *name;
    unsigned long count;
} *datasets = (struct dataset_type *) 0,
  *datasets_tail = (struct dataset_type *) 0;

static struct datalist_type {
    struct datalist_type *next;
    char *line;
} *datalist = (struct datalist_type *) 0,
  *datalist_tail = (struct datalist_type *) 0;

static struct dataset_name_type {
    struct dataset_name_type *next;
    char *name;
} *dataset_names = (struct dataset_name_type *) 0;

static BOOLEAN dataname_flag = FALSE;  /* set if dataname pseudo-op is found */

/*

    get_dataname()

    Given a dataname number, return the name associated with it. This
    routine assumes that the index corresponds to an existing dataname:
    no error checking is done for an out-of-range value.

*/

char *get_dataname(index)
register unsigned long index;
{
    register struct dataname_type *current_dataname;

    current_dataname = datanames;
    while (index--) current_dataname = current_dataname->next;
    return current_dataname->name;
}

/*

    get_dataname_index()

    Given a dataname, return the index associated with it. If it is not
    found, return 0xffffffff.

*/

unsigned long get_dataname_index(ptr)
{
    register struct dataname_type *current_dataname;
    unsigned long count;

    current_dataname = datanames;
    count = 0;
    while (current_dataname != (struct dataname_type *) 0) {
	if (!strcmp(current_dataname->name, ptr)) 
	    return count;
	current_dataname = current_dataname->next;
        count++;
    };

    return 0xffffffff;
}

/*

    get_dataset_name()

    Given a dataset number, return the name associated with it. This
    routine assumes that the index corresponds to an existing dataset:
    no error checking is done for an out-of-range value.

*/

char *get_dataset_name(index)
register unsigned long index;
{
    register struct dataset_type *current_dataset;

    current_dataset = datasets;
    while (index--) current_dataset = current_dataset->next;
    return current_dataset->name;
}

/*

    get_datum()

    Given a dataset number and a dataname number, return the datum
    associated with them.  This routine assumes that the index
    corresponds to an existing dataname/dataset pair:  no error checking
    is done for out-of-range values.

*/

char *get_datum(dataname, dataset)
register unsigned long dataname, dataset;
{
    register struct dataset_type *current_dataset;
    register struct datum_type *current_datum;

    current_dataset = datasets;
    while (dataset--) current_dataset = current_dataset->next;
    current_datum = current_dataset->data;
    while (dataname--) current_datum = current_datum->next;
    return current_datum->datum;
}

/*

    max_dataset_count()

    Return the largest single count of all data set counts. If there
    are no datasets, the value returned is 1.

*/

unsigned long max_dataset_count()
{
    register struct dataset_type *current_dataset;
    unsigned long max;

    max = 1;
    current_dataset = datasets;
    while (current_dataset != (struct dataset_type *) 0) {
	if (current_dataset->count > max) max = current_dataset->count;
	current_dataset = current_dataset->next;
    };
    return max;
}

/*

    sum_dataset_count()

    Return the sum of all dataset counts. An error condition is noted if
    the count overflows the range of an unsigned long. If there are no
    datasets, the value returned is 1.

*/

unsigned long sum_dataset_count()
{
    register struct dataset_type *current_dataset;
    unsigned long sum;

    sum = 0;
    if ((current_dataset = datasets) == (struct dataset_type *) 0)
	return 1;
    do {
	if (current_dataset->count > (0xFFFFFFFF - sum)) {
	    error(9, (char *) 0);
	    return 0xFFFFFFFF;
	};
	sum += current_dataset->count;
	current_dataset = current_dataset->next;
    } while (current_dataset != (struct dataset_type *) 0);
    return sum;
}

/*

    Dprint_count_table()

    Print a count table. This table contains the counts associated 
    with datasets.

    #if there is more than one data set

		data
		lalign	4

     #if the largest single count > 65535
	count:	
		long	{count0}
		long	{count1}
		long	{count2}
		long	{count3}
		long	{count4}
		long	{count5}
		long	{count6}
		long	{count7}
     #else
	count:	
		short	{count0}
		short	{count1}
		short	{count2}
		short	{count3}
		short	{count4}
		short	{count5}
		short	{count6}
		short	{count7}
     #endif

    #endif

*/

void Dprint_count_table()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register struct dataset_type *current_dataset;

    if (gDatasetCount > 1) {
	print(output, "\tdata\n");
	print(output, "\tlalign\t4\n");
	print(output, "count:\n");
	current_dataset = datasets;
	if (max_dataset_count() > 65535) {
	    while (current_dataset != (struct dataset_type *) 0) {
		print(output, "\tlong\t%u\n", current_dataset->count);
		current_dataset = current_dataset->next;
	    };
	} else {
	    while (current_dataset != (struct dataset_type *) 0) {
		print(output, "\tshort\t%u\n", current_dataset->count);
		current_dataset = current_dataset->next;
	    };
	};
    };

    return;
}

/*

    Dprint_dataset_table_code()

    Create code to print the dataname/dataset table for execution
    profiling.  This table information comes directly from the dataname
    and dataset instructions themselves.  As an example of such a table:

			$number
		bit7,	0x80
		bit6,	0x40
		bit5,	0x20
		bit4,	0x10
		bit3,	0x08
		bit2,	0x04
		bit1,	0x02
		bit0,	0x01
		zero,	0x00

			---*---

		data
    #if there are datasets
	dsets:	
		byte	10			# extra linefeed
		byte	{data set information}
		byte	{data set information}
		byte	{data set information}
		byte	{data set information}
		byte	{data set information}
		byte	{data set information}
		byte	{data set information}
		byte	{data set information}
    #else
	dsets: 
    #endif
		byte	10			# extra linefeed

		text
		mov.l	file,-(%sp)
		pea	{length}		# length of table information
		pea	1.w
		pea	dsets
		jsr	_fwrite			# fwrite(dsets,1,{length},file)
		lea	16(%sp),%sp

*/

void Dprint_dataset_table_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    unsigned long length;
    struct datalist_type *line;

    line = datalist;
    length = 0;
    print(output, "\tdata\n");
    if (gDatasetCount) {
        print(output, "dsets:");
	print(output, "\tbyte\t10\n");
	length++;
        while (line != (struct datalist_type *) 0) {
	    length += strlen(line->line);
	    print_byte_info(line->line);
	    line = line->next;
	};
    } else {
        print(output, "dsets:");
    };
    print(output, "\tbyte\t10\n");
    length++;
    print(output, "\ttext\n");
    print(output, "\tmov.l\tfile,-(%%sp)\n");
    print(output, "\tpea\t%d\n", length);
    print(output, "\tpea\t1.w\n");
    print(output, "\tpea\tdsets\n");
    print(output, "\tjsr\t_fwrite\n");
    print(output, "\tlea\t16(%%sp),%%sp\n");

    return;
}

/*

    instr_dataname()

    Process a "dataname" pseudo-op. This routine is called when a 
    "dataname" is encountered in the input file. It is expecting the
    variable "ptr" to point to the first character in the input line
    which follows the dataname mnemonic.

*/

void instr_dataname(ptr)
register char *ptr;
{
    register int c;
    char *tk_start, *tk_end;
    unsigned long comma_count, length;
    struct dataname_type *current_dataname, *last_dataname;
    BOOLEAN bad_name;

    /* "dataname" has been found */
    dataname_flag = TRUE;		

    /* must be in fizz initialization section */
    if (gSection != FIZZ_INIT) {	
	error_locate(11);
	return;
    };

    /* error if there's been a previous dataname */
    if (datanames != (struct dataname_type *) 0) {
	error_locate(19);
	return;
    };

    comma_count = 0;
    last_dataname = (struct dataname_type *) 0;
    bad_name = FALSE;

    for (;;) {
	get_next_list_item(ptr, &tk_start, &tk_end);  /* get next dataname */
	if (!tk_start) break;			      /* done if end-of-line */
	if (*tk_start == '#') {		/* check for comment */
	    error_locate(116);
	    break;
	};
	ptr = tk_start;
	if (*ptr++ != '$') {		/* dataname must start with '$' */
	    if (!bad_name) {
	        error_locate(5);
		bad_name = TRUE;
	    };
	} else {
	    while (ptr < tk_end)	/* make sure it is a valid name */
	        if (!(isalnum(c = *ptr++) || (c == '_'))) {
		    if (!bad_name) {
			error_locate(5);
			bad_name = TRUE;
		    };
		    break;
	        };
	    if (ptr == tk_end) {
		current_dataname = datanames;

		/* check for duplicate dataname */
		while (current_dataname != (struct dataname_type *) 0) {
		    if ((strlen(current_dataname->name) == (tk_end - tk_start)) 
		      && (!strncmp(current_dataname->name, tk_start,
		      tk_end - tk_start))) {
			error_locate(122);
			break;
		    };
		    current_dataname = current_dataname->next;
		};

		/* add new dataname to the end of the list */
		CREATE_STRUCT(current_dataname, dataname_type);
		CREATE_STRING(current_dataname->name, tk_end - tk_start + 1);
		strncpy(current_dataname->name, tk_start, tk_end - tk_start);
		current_dataname->name[tk_end - tk_start] = '\0';
		current_dataname->next = (struct dataname_type *) 0;
		if (last_dataname == (struct dataname_type *) 0)
		    datanames = current_dataname;
		else last_dataname->next = current_dataname;
		last_dataname = current_dataname;
		gDatanameCount++;
	    } else ptr = tk_end;
	};
	if (*ptr == ',') {		/* does name end with comma? */
	    comma_count++;
	    ptr++;
	};
    };
    if (!bad_name) {
	if (!gDatanameCount) error_locate(21);		/* null list? */
	else if (comma_count >= gDatanameCount)		/* too many commas? */
	    error_locate(28);
	else if (comma_count < (gDatanameCount - 1))	/* too few commas? */
	    error_locate(31);
    };

    /* 
	If this is execution profiling, we need to add this instruction
	to the end of the lines to be printed to the output.  See
	Dprint_dataset_table_code().
    */
    if (gMode == EXECUTION_PROFILING) {
	CREATE_STRUCT(datalist, datalist_type);
        length = strlen(gBuffer);
	CREATE_STRING(datalist->line, length + 2);
        (void) strcpy(datalist->line, gBuffer);
	/* blank out the word "dataname" */
        (void) strncpy(&datalist->line[ptr - 8 - gBuffer], "        ", 8);
        datalist->line[length] = '\n';
        datalist->line[length + 1] = '\0';
	datalist->next = (struct datalist_type *) 0;
        datalist_tail = datalist;
    };

    return;
}

/*

    instr_dataset()

    Process a "dataset" pseudo-op. This routine is called whenever a
    "dataset" is encountered in the input file.

*/

void instr_dataset(ptr)
register char *ptr;
{
    struct dataset_type *current_dataset;
    struct datalist_type *current_datalist;
    struct datum_type *current_datum, *datum_tail;
    struct dataset_name_type *current_dataset_name;
    unsigned long datacount, count, comma_count, length;
    int c;
    char *tk_start, *tk_end, *name_end, *op_end;
    
    /* error if not in fizz initialization */
    if (gSection != FIZZ_INIT) {    
	error_locate(12);
	return;
    };

    /* error if no dataname instruction */
    if (!dataname_flag) {
	error_locate(13);
	return;
    };

    op_end = ptr;
    get_next_list_item(ptr, &tk_start, &tk_end);	/* get name */
    if (tk_start == (char *) 0) {	/* error if no name */
	error_locate(17);
	return;
    };
    if (*tk_start == '#') {		/* error if comment */
	error_locate(117);
	error_locate(17);
	return;
    };

    ptr = tk_start;
    name_end = tk_end;
    if (!isalpha(*ptr++)) {		/* check for valid initial char */
	error_locate(6);
	return;
    } else {
	while (isalnum(c = *ptr++) || (c == '_')); /* scan to end of name */
	count = 1;
	if (ptr <= tk_end) {		/* possibly a count? */
	    name_end = ptr - 1;
	    if (c == '(') {		/* check for parentheses */
		if (*(tk_end - 1) == ')') {
		    if (ptr == tk_end - 1)
			error_locate(16);
		    else {
			count = 0;	/* for accumulating count */
			while (ptr < tk_end - 1) {
			    if (!isdigit(c = *ptr++)) {	/* digit? */
				error_locate(8);
				count = 1;
				break;
			    };
			    if ((count > 0x19999999) ||  /* overflow? */
			      ((count == 0x19999999) && (c >= '6'))) {
				error_locate(10);
				count = 1;
				break;
			    };
			    count += count;	/* count = 10 * count + digit */
			    count += count << 2;
			    count += c - '0';
			};
			if (ptr < tk_end - 1) count = 1; /* was a problem? */
		    };
		} else error_locate(18); /* missing ')' */
	    } else {
		error_locate(6);	/* bad character in count */
		return;
	    };
	};
    };
    if (!count) error_locate(115);	/* zero count */
    CREATE_STRING(ptr, name_end - tk_start + 1); /* get space for name */
    strncpy(ptr, tk_start, name_end - tk_start);
    ptr[name_end - tk_start] = '\0';
    current_dataset_name = dataset_names;

    /* search for duplicate dataset name */
    while (current_dataset_name != (struct dataset_name_type *) 0) {
	if (!strcmp(ptr, current_dataset_name->name)) {
	    error_locate(121);
	    free(ptr);
	    return;
	};
	current_dataset_name = current_dataset_name->next;
    };

    /* create space for dataset information */
    CREATE_STRUCT(current_dataset_name, dataset_name_type);
    CREATE_STRING(current_dataset_name->name, strlen(ptr) + 1);
    (void) strcpy(current_dataset_name->name, ptr);
    current_dataset_name->next = dataset_names;
    dataset_names = current_dataset_name;

    /* verify that this dataset is needed */
    if ((gMode == PERFORMANCE_ANALYSIS) || search_list(ptr)) {
	CREATE_STRUCT(current_dataset, dataset_type);
	current_dataset->next = (struct dataset_type *) 0;
	current_dataset->name = ptr;
	current_dataset->count = count;
	current_dataset->data = (struct datum_type *) 0;
    } else {
	free(ptr);
	free(current_dataset_name->name);
	free(current_dataset_name);
	return;
    };

    comma_count = 0;
    datacount = 0;
    datum_tail = (struct datum_type *) 0;
    for (;;) {
        ptr = tk_end;
        if (*ptr == ',') {
	    comma_count++;
	    ptr++;
        };
	get_next_list_item(ptr, &tk_start, &tk_end);	/* get next datum */
	if (!tk_start) break;			/* done? */
	if (*tk_start == '#') {			/* comment? */
	    error_locate(117);
	    break;
	};
	CREATE_STRING(ptr, tk_end - tk_start + 1);	/* space for datum */
	(void) strncpy(ptr, tk_start, tk_end - tk_start);
	ptr[tk_end - tk_start] = '\0';
	CREATE_STRUCT(current_datum, datum_type);
	current_datum->datum = ptr;
	current_datum->next = (struct datum_type *) 0;
	if (datum_tail == (struct datum_type *) 0)	/* link it in */
	    current_dataset->data = current_datum;
	else datum_tail->next = current_datum;
	datum_tail = current_datum;
	datacount++;
    };

    /* link in new dataset */
    if (datasets == (struct dataset_type *) 0) datasets = current_dataset;
    else datasets_tail->next = current_dataset;
    datasets_tail = current_dataset;
    if (comma_count != datacount) {		/* wrong number of commas? */
	if (comma_count < datacount) error_locate(32);
	else error_locate(29);
    };

    /* datum count match dataname count? */
    if (datacount != gDatanameCount) error_locate(114);

    gDatasetCount++;	/* one more dataset */

    /* 
	If this is execution profiling, we need to add this instruction
	to the end of the lines to be printed to the output.  See
	Dprint_dataset_table_code().
    */
    if (gMode == EXECUTION_PROFILING) {
	CREATE_STRUCT(current_datalist, datalist_type);
        length = strlen(gBuffer);
	CREATE_STRING(current_datalist->line, length + 2);
        (void) strcpy(current_datalist->line, gBuffer);
	/* blank out the word "dataset" */
	(void) strncpy(&current_datalist->line[op_end - 7 - gBuffer], 
	  "       ", 7);
        current_datalist->line[length] = '\n';
        current_datalist->line[length + 1] = '\0';
	current_datalist->next = (struct datalist_type *) 0;
	if (datalist == (struct datalist_type *) 0) datalist = current_datalist;
	else datalist_tail->next = current_datalist;
        datalist_tail = current_datalist;
    };
    return;
}
