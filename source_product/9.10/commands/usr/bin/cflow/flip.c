static char *HPUX_ID = "@(#) $Revision: 27.1 $";
#include <stdio.h>
#include <ctype.h>

main()
	{
	char line[BUFSIZ], *pl, *gets();

	while (pl = gets(line))
		{
		while (*pl != ':')
			++pl;
		*pl++ = '\0';
		while (isspace(*pl))
			++pl;
		printf("%s : %s\n", pl, line);
		}
	}
