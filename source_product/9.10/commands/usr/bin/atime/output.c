/*

    open_output_file()

    Open a file for output.

*/

#include "fizz.h"

void open_output_file(filename)
char *filename;
{
    if ((gOutput = fopen(filename, "a")) == NULL)
	error(63, filename);
    return;
}

/*

    close_output_file()

    Close the current output file.

*/

void close_output_file(filename)
{
    if (fclose(gOutput) == EOF) error(61, filename);
    gOutput = (FILE *) 0;
    return;
}
