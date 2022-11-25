static char *HPUX_ID = "@(#) $Revision: 64.1 $";

#include <pwd.h>
#include <stdio.h>

/*
 * prmail
 */
struct	passwd *getpwuid();
char	*getenv();

main(argc, argv)
	int argc;
	char **argv;
{
	static char MAILDIR[] = "/usr/mail";
	register struct passwd *pp;

	--argc, ++argv;
	if (chdir(MAILDIR) < 0) {
		perror(MAILDIR);
		exit(1);
	}
	if (argc == 0) {
		char *user = getenv("USER");
		if (user == 0) {
			pp = getpwuid(getuid());
			if (pp == 0) {
				fputs("Who are you?\n", stdout);
				exit(1);
			}
			user = pp->pw_name;
		}
		prmail(user, 0);
	} else
		while (--argc >= 0)
			prmail(*argv++, 1);
	exit(0);
}

#include <sys/types.h>
#include <sys/stat.h>

prmail(user, other)
	char *user;
{
	struct stat stb;
	char cmdbuf[256];

	if (stat(user, &stb) < 0) {
		fputs("No mail for ", stdout);
		fputs(user, stdout);
		fputs(".\n", stdout);
		return;
	}
	if (access(user, 4) < 0) {
		fputs("Mailbox for ", stdout);
		fputs(user, stdout);
		fputs(" unreadable\n", stdout);
		return;
	}
	if (other) {
		fputs(">>> ", stdout);
		fputs(user, stdout);
		fputs(" <<<\n", stdout);
	}
	strcpy(cmdbuf, "more ");
	strcat(cmdbuf, user);
	system(cmdbuf);
	if (other)
		fputs("-----\n\n", stdout);
}
