/* @(#) $Revision: 64.1 $ */   
#include <sys/param.h>
#include "ptyrequest.h"

char *
getnodename()
{
    static char name[MAXHOSTNAMELEN];

    if (gethostname(name, sizeof(name)) == -1)
	return((char *)0);
    return(name);
}
