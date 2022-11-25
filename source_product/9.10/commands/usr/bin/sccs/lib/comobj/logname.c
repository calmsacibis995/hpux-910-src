/* @(#) $Revision: 37.1 $ */   
# include	<pwd.h>
# include	<sys/types.h>
# include	<macros.h>

char	*logname()
{
	struct passwd *getpwuid();
	struct passwd *log_name;
	int uid;

	uid = getuid();
	log_name = getpwuid(uid);
	endpwent();
	if (! log_name)
		return(0);
	else
		return(log_name->pw_name);
}
