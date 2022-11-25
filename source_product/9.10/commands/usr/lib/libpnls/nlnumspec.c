


#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <langinfo.h>
#include <nlinfo.h>

#define MAXCURRINFO		21

void nlnumspec(langid, numspec, err)
short		langid;
unsigned short	err[];
char	*numspec;
{
	struct l_info *n;
	struct l_info *getl_info();

	err[0] = err[1] = 0;

	if (langid == -1)  {
		err[0] = E_LNOTCONFIG;
                return;
        }

	if (n = getl_info(langid))
		memcpy(numspec, n->numspec, SIZE_NUMSPEC);
	else
		err[0] = E_LNOTCONFIG;
}
