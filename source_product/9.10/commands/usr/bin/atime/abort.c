#include "fizz.h"

/*

    abort()

    Last chance cleanup before aborting because of an error.

*/

void abort()
{
    cleanup(TRUE);
    exit(1);
}

/*

    signal_abort()

    This routine is called when a signal occurs.

*/

void signal_abort()
{
    gfMessages = FALSE;	/* quiet cleanup: don't print any error messages */
    abort();
}
