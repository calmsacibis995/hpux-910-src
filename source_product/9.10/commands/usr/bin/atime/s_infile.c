/*

    store_input_filename()

    Store the input file name.

*/

#include "fizz.h"

void store_input_filename(filename)
char *filename;
{
    CREATE_STRING(gInputFile, strlen(filename) + 1);
    (void) strcpy(gInputFile, filename);

    return;
}
