/* @(#) $Revision: 64.3 $ */   
#ifdef _NAMESPACE_CLEAN
#define environ _environ
#define execl _execl
#define execve _execve
#endif

extern char	**environ;

#ifdef _NAMESPACE_CLEAN
#undef execl
#pragma _HP_SECONDARY_DEF _execl execl
#define execl _execl
#endif

execl(name, argl)
char	*name;
char	*argl;
{
	return(execve(name, &argl, environ));
}
