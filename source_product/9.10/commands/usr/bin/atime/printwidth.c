/*

    printwidth()

    Calculate the maximum size of all datanames and assertion names.

*/

#include "fizz.h"

unsigned long printwidth()
{
    static unsigned long width = 0;
    unsigned long count, name_length;

    if (!width) {	/* has this been calculated before? */

	/* find maximum length of datanames */
	for (count = 0; count < gDatanameCount; count++) {
	    name_length = strlen(get_dataname(count));
	    if (name_length > width) width = name_length;
	};

	/* find maximum length of datanames and assertion names */
	for (count = 0; count < gAssertionCount; count++) {
	    name_length = strlen(get_assertion_name(count));
	    if (name_length > width) width = name_length;
	};

    };

    return width;
};
