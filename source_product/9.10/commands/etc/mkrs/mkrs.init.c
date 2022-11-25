static char *HPUX_ID = "@(#) $Revision: 66.3 $";
/* HPUX_ID: @(#) $Revision: 66.3 $  */
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

main()
{
	int childpid, pid, status;
	FILE *fp;

	setpgrp();
	close(0);
	open("/dev/console", O_RDWR);
	close(1);
	dup(0);	
	close(2);
	dup(0);

	/* ignore ^C or BREAK signal */
	signal (SIGINT, SIG_IGN);

	/* ignore ^\ kill signal */
	signal (SIGQUIT, SIG_IGN);

	while (1) {
		childpid = fork();
		if (childpid == 0) {
			if (access("/etc/-recovery.tool",X_OK)==0)
				execlp ("/etc/-recovery.tool", "/etc/-recovery.tool", 0);
			else
				execlp ("/bin/sh", "-sh", 0);
			exit (1);
		}

		do {
			pid = wait(&status);
		}
		while (pid != childpid);
	}
}
