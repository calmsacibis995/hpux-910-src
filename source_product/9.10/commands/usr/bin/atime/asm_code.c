/*

    assemble_code()

    Assemble the code file.

*/

#include "fizz.h"

void assemble_code()
{
    char *command;
    int status;

    /* Create a temporary file name */
    CREATE_STRING(gCodeObjFile, L_tmpnam);
    (void) tmpnam(gCodeObjFile);

    /* Create the assembler command and execute it */
    CREATE_STRING(command, strlen(gCodeFile) + strlen(gCodeObjFile) + 14);
    sprintf(command, "/bin/as %s -o %s\n", gCodeFile, gCodeObjFile);
    status = system(command);
    free(command);
    if (status) {
        command = gCodeFile;
        gCodeFile = (char *) 0;
	error(60, command);
    }

    return;
}
