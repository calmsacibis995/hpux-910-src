/* @(#) $Revision: 70.1 $ */

#ifdef _NAMESPACE_CLEAN
#define strncpy _strncpy
#define strlen _strlen
#endif /* _NAMESPACE_CLEAN */

#include <unistd.h>
#include <errno.h>
#define NULL 0
#define xcpy(a,b,c)	if (a) strncpy(a,b,c)

#ifdef _NAMESPACE_CLEAN
#undef confstr
#pragma _HP_SECONDARY_DEF _confstr confstr
#define confstr _confstr
#endif /* _NAMESPACE_CLEAN */

size_t confstr(name,buf,len)
int name;
char *buf;
size_t len;
{
	switch(name) {
		case _CS_PATH:
			xcpy(buf,CS_PATH,len-1);
			return sizeof CS_PATH;
	}
	errno = EINVAL;
	return 0;
}
