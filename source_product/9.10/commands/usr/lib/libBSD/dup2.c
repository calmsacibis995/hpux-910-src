/* @(#) $Revision: 27.1 $ */    
#include <fcntl.h>
int dup2(f1,f2)
int f1,f2;
{
	close(f2);
	return fcntl(f1, F_DUPFD, f2);
}

