/*

    assemble_driver()

    Assemble the driver file.

*/

#include "fizz.h"

void assemble_driver()
{
    char *command;
    int status;

    /* Create a temporary file name */
    CREATE_STRING(gDriverObjFile, L_tmpnam);
    (void) tmpnam(gDriverObjFile);

    /* Create the assembler command and execute it */
    CREATE_STRING(command, strlen(gDriverFile) + strlen(gDriverObjFile) + 14);
    sprintf(command, "/bin/as %s -o %s\n", gDriverFile, gDriverObjFile);
    status = system(command);
    free(command);

    if (status) error(60, gDriverFile);

    return;
}
