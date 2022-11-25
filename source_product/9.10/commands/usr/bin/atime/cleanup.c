/*

    cleanup()

    Remove temporary files.

*/

#include "fizz.h"

void cleanup(flag)
BOOLEAN flag;
{
    if (gFile != (char *) 0) close_input_files();
    if (gOutput != (FILE *) 0) close_output_file("");

    /* remove driver source file */
    if (gDriverFile != (char *) 0) {
	(void) unlink(gDriverFile);
	free(gDriverFile);
	gDriverFile = (char *) 0;
    };

    /* remove driver object file */
    if (gDriverObjFile != (char *) 0) {
	(void) unlink(gDriverObjFile);
	free(gDriverObjFile);
	gDriverObjFile = (char *) 0;
    };

    /* remove assembly source file */
    if (gCodeFile != (char *) 0) {
	(void) unlink(gCodeFile);
	free(gCodeFile);
	gCodeFile = (char *) 0;
    };

    /* remove assembly object file */
    if (gCodeObjFile != (char *) 0) {
	(void) unlink(gCodeObjFile);
	free(gCodeObjFile);
	gCodeObjFile = (char *) 0;
    };

    /* remove executable file */
    if (flag && gExecFile != (char *) 0) {
	(void) unlink(gExecFile);
	free(gExecFile);
	gExecFile = (char *) 0;
    };

    return;
}
