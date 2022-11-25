/* $Revision: 64.1 $ */
/* findtty(user) -- find first tty that user is logged in to.
	returns: /dev/tty?? if user is logged in
		 NULL, if not
*/

#include	"lp.h"
#include	"lpsched.h"

char *
findtty(user)
char *user;
{
	struct utmp utmp;
	static char tty[NAMEMAX + 5];
	FILE *u;

	if((u = fopen(UTMP, "r")) == NULL)
		return(NULL);
	while(fread((char *) &utmp, sizeof(struct utmp), 1, u) == 1){
		if(utmp.ut_type == USER_PROCESS){
			if(strcmp(utmp.ut_name, user) == 0) {
				sprintf(tty, "/dev/%s", utmp.ut_line);
				fclose(u);
				return(tty);
			}
		}
	}
	fclose(u);
	return(NULL);
}
