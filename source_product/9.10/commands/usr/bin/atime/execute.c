/*

    execute()

    Execute the linked executable file.

*/

#include "fizz.h"

void execute()
{
    cleanup(FALSE);
    (void) execl(gExecFile, 0);
    error(66, (char *) 0);
    return;
}
