/* @(#) $Revision: 26.1 $ */      
char *strend(p)
register char *p;
{
	while (*p++)
		;
	return(--p);
}
