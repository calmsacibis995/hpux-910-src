/* @(#) $Revision: 66.3 $ */
/*
 * getccent() -- return one entry from the /etc/clusterconf file
 * setccent() -- open (or rewind) /etc/clusterconf
 * endccent() -- close /etc/clusterconf
 *               see getccent(3C).
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define endccent _endccent
#define fgetccent _fgetccent
#define getccent _getccent
#define setccent _setccent
#define fopen _fopen
#define fclose _fclose
#define rewind _rewind
#endif

#include <stdio.h>
#include <sys/types.h>
#include <cluster.h>

extern struct cct_entry *fgetccent();

static char CLUSTCONF[] = "/etc/clusterconf";
static FILE *clustf = NULL;

#ifdef _NAMESPACE_CLEAN
#undef getccent
#pragma _HP_SECONDARY_DEF _getccent getccent
#define getccent _getccent
#endif /* _NAMESPACE_CLEAN */

struct cct_entry *
getccent()
{
    if (clustf == NULL)
    {
	if ((clustf = fopen(CLUSTCONF, "r")) == NULL)
	    return NULL;
    }
    return fgetccent(clustf);
}

#ifdef _NAMESPACE_CLEAN
#undef setccent
#pragma _HP_SECONDARY_DEF _setccent setccent
#define setccent _setccent
#endif /* _NAMESPACE_CLEAN */

void
setccent()
{
    if (clustf == NULL)
	clustf = fopen(CLUSTCONF, "r");
    else
	rewind(clustf);
}

#ifdef _NAMESPACE_CLEAN
#undef endccent
#pragma _HP_SECONDARY_DEF _endccent endccent
#define endccent _endccent
#endif

void
endccent()
{
    if (clustf != NULL)
    {
	(void)fclose(clustf);
	clustf = NULL;
    }
}
