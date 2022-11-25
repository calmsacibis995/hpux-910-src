/* @(#) $Revision: 37.1 $ */    

extern int errno;
main(ac,av)
int ac;
char **av;
{
	errno = atoi(av[1]);

	perror("");
}
