
/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define endccent _endccent
#define getccent _getccent
#define setccent _setccent
#define strcmp _strcmp
#define getccnam _getccnam
#endif

#include <sys/types.h>
#include <cluster.h>
#include <stdio.h>

struct cct_entry *getccent();
void setccent(), endccent();

#ifdef _NAMESPACE_CLEAN
#undef getccnam
#pragma _HP_SECONDARY_DEF _getccnam getccnam
#define getccnam _getccnam
#endif /* _NAMESPACE_CLEAN */

struct cct_entry *
getccnam(name)
char *name;
{
	struct cct_entry *p;

	if (name ==NULL || *name == '\0')
		return (NULL);
	
	setccent();
	while ((p = getccent()) && strcmp(name,p->cnode_name))
		;
	endccent();
	return (p);
}
