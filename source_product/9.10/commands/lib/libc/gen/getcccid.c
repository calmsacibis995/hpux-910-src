
/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define endccent _endccent
#define setccent _setccent
#define getccent _getccent
#define getcccid _getcccid
#endif

#include <sys/types.h>
#include <cluster.h>
#include <stdio.h>

struct cct_entry *getccent();
void setccent(), endccent();

#ifdef _NAMESPACE_CLEAN
#undef getcccid
#pragma _HP_SECONDARY_DEF _getcccid getcccid
#define getcccid _getcccid
#endif /* _NAMESPACE_CLEAN */

struct cct_entry *
getcccid(cid)
cnode_t cid;
{
	struct cct_entry *p;

	setccent();
	while ((p = getccent()) && cid != p->cnode_id)
		;
	endccent();
	return (p);
}
