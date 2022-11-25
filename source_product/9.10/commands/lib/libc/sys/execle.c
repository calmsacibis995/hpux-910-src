/* @(#) $Revision: 64.3 $ */   

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _execle execle
#define execle _execle
#define execve _execve
#endif

execle(name, argl)
char	*name;
char	*argl;
{
	char    ***envpptr = (char ***) &argl;

	while (*envpptr++)
		;
	return(execve(name, &argl, *envpptr));
}

