/* @(#) $Revision: 26.1 $ */    


char *move(a,b,n)
char *a, *b;
unsigned n;
{
	while(n--) *b++ = *a++;
}
