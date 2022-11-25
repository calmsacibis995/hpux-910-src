/*
 * @(#) $Revision: 63.1 $
 */

#define	reg		register

extern char *		strrchr();

char *
basename(string)
char *string;
{

	reg	char *		ptr = (char *)0;


	if ((ptr = strrchr(string, '/')) != (char *)0) {
		return(++ptr);
	}
	return(string);
}
