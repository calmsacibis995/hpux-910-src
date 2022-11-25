
#ifdef _NAMESPACE_CLEAN
#define kill _kill
#define getpid _getpid
#define raise _raise
#endif

#include <signal.h>

extern int kill(), getpid();

#ifdef _NAMESPACE_CLEAN
#undef raise
#pragma _HP_SECONDARY_DEF _raise raise
#define raise _raise
#endif

int
raise (sig)
int sig;
{
	return (kill(getpid(),sig));
}
