/*

    calculate_iterations()

    Given the number of iterations and the total count of the datasets,
    calculate the true number of iterations.

*/

#include "fizz.h"

void calculate_iterations()
{
    unsigned long count;
    double iterations;

    if (gDatasetCount) {
	count = sum_dataset_count();
	if (gIterations % count) {
	    iterations = (double) ((gIterations / count) + 1);
	    iterations *= (double) count;
	    if (iterations > 4294967295.0)	/* overflow? */
		error(14, (char *) 0);
	    else
		gIterations = (unsigned long) iterations;
	};
    };

    return;
}
