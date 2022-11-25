#include "fizz.h"

/*

     Perform various operations on dataset names specified from the
     command line.

		     +-----------+   +-----------+   +-----------+
      dataset_list-->|   next    |-->|   next    |-->| next = 0  |
		     +-----------+   +-----------+   +-----------+
		     |   name    |   |   name    |   |    name   |
		     +----+------+   +-----------+   +-----------+
			  |		  |		   |
			  V		  V		   V
		     "dataset name 1"  "dataset name 2" "dataset name 3"

     
*/

static unsigned long named_dataset_count = 0;
static unsigned long unnamed_dataset_count = 0;
static BOOLEAN dataset_all_flag = FALSE;
static struct listtype {
    struct listtype *next;
    char *name;
} *dataset_list = (struct listtype *) 0;

/*

    store_list_name()

    Add to list of datasets referenced from the command line.

*/

void store_list_name(mode, name)
short mode;
char *name;
{
    register int c;
    register char *ptr;
    struct listtype *listptr;

    /* are modes all consistent? */
    if (gMode == PERFORMANCE_ANALYSIS) gMode = mode;
    else if (mode != gMode) error(3, (char *) 0);

    ptr = name;
    if (!*ptr) {	/* generic "-p" or "-l" ? */
	dataset_all_flag = TRUE;
	return;
    };

    /* verify that dataset name is valid */
    if (!isalpha(*ptr++)) {
	error(7, (char *) 0);
	return;
    };
    while (c = *ptr++)
        if (!(isalnum(c) || (c == '_'))) {
	    error(7, (char *) 0);
	    return;
        };

    /* check for duplicate name */
    listptr = dataset_list;
    while (listptr != (struct listtype *) 0) {
	if (!strcmp(listptr->name, name)) return;
	listptr = listptr->next;
    };

    /* create space for next dataset name */
    CREATE_STRUCT(listptr, listtype);
    if ((listptr->name = malloc(strlen(name) + 1)) == (char *) 0) {
	free(listptr);
	error(2, (char *) 0);
    };

    /* add name to list */
    (void) strcpy(listptr->name, name);
    listptr->next = dataset_list;
    dataset_list = listptr;
    named_dataset_count++;

    return;
}

/*

    search_list()

    Search the list of datasets referenced from the command line for the 
    given dataset name.

    It is assumed that this routine is only called once for each dataset
    encountered in the input file. If the name was not specified from the
    command line, an unnamed dataset count is incremented. After all is
    done, the number of named datasets plus the number of unnamed data sets
    must equal the total number of datasets. If not, there was an undefined
    dataset specified on the command line.

    This routine will always return TRUE if there was a vanilla "-l" or
    "-p" on the command line.

*/

BOOLEAN search_list(name)
char *name;
{
    BOOLEAN found;
    struct listtype *listptr;

    found = FALSE;
    listptr = dataset_list;
    while (listptr != (struct listtype *) 0) {
	if (!strcmp(listptr->name, name)) {
	    found = TRUE;
	    break;
	};
	listptr = listptr->next;
    };

    if (dataset_all_flag && !found) ++unnamed_dataset_count;

    return found || dataset_all_flag;
};

/*

    verify_list()

    Verify that all datasets referenced from the command line are defined
    in the input file. Print a message for any that are not.

*/

void verify_list()
{
    unsigned long count;
    struct listtype *listptr;

    if ((named_dataset_count + unnamed_dataset_count) != gDatasetCount) {
        listptr = dataset_list;
        while (listptr != (struct listtype *) 0) {
	    for (count = 0; count < gDatasetCount; count++)
		if (!strcmp(listptr->name, get_dataset_name(count))) break;
	    if (count == gDatasetCount)
		error(44, listptr->name);
	    listptr = listptr->next;
        };
	abort();
    };

    return;
}
