static char *HPUX_ID = "@(#) $Revision: 70.1 $";

#include <stdio.h>
#include <nl_types.h>
#define NL_SETN 1
main() {
	char *name, *getlogin();
	nl_catd catd;

	catd = catopen("logname",0);
	name = getlogin();
	if (name == NULL) {
		fputs("logname: ",stderr);
		fputs(catgets(catd,NL_SETN,1,"could not get login name\n"),stderr);
		return (1);
	}
	(void) puts (name);
	return (0);
}
