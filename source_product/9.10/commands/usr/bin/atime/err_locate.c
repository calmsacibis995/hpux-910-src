/*

    error_locate()

    Pass an error number to the error routine along with a string
    indicating file and line number.

*/

#include "fizz.h"

void error_locate(error_number)
int error_number;
{
    char *buffer;

    CREATE_STRING(buffer, strlen(gFile) + 20);
    sprintf(buffer, "\"%s\", line %u", gFile, gLine);
    error(error_number, buffer);
    free(buffer);

    return;
}
