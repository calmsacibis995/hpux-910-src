static char *HPUX_ID = "@(#) $Revision: 64.2 $";

#include <stdio.h>
#include <pwd.h>
/*
 * whoami
 */
struct	passwd *getpwuid();

main()
{
	register struct passwd *pp;

	pp = getpwuid(geteuid());
	if (pp == 0) {
		fputs("Intruder alert.\n", stdout);
		exit(1);
	}
	fputs(pp->pw_name, stdout);
	fputc('\n', stdout);
	exit(0);
}
