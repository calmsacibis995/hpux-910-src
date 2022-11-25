/* @(#) $Revision: 64.4 $ */   

#ifdef _NAMESPACE_CLEAN
#define environ _environ
#define execv _execv
#define execve _execve
#endif

extern char	**environ;

#ifdef _NAMESPACE_CLEAN
#undef execv 
#pragma _HP_SECONDARY_DEF _execv execv
#define execv _execv
#endif

execv(name, argv)
char	*name;
char	**argv;
{
	return(execve(name, argv, environ));
}
