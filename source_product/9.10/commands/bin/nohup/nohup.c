#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
nl_catd catd;
#endif NLS

static char *HPUX_ID = "@(#) $Revision: 70.3 $";

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>


char	nout[100] = "nohup.out";
char	*getenv();

main(argc, argv)
char **argv;
{
	char	*home;
	FILE    *temp;
	int	err;
	int     flag;
        int     oldmask;

#ifdef NLS
	catd=catopen("nohup",0);
#endif

	if(argc < 2) {
		fputs((catgets(catd,NL_SETN,101, "usage: nohup command arg ...\n")), stderr);
		exit(127);
	}
	argv[argc] = 0;
	signal(SIGHUP, SIG_IGN);
	/*
	 *   signal(SIGQUIT, SIG_IGN);
	 *     removed to be XPG4 draft 4 compliant.
	 */
	if(isatty(1)) {
		/*
		 *   XPG4 requires the permissions on nohup.out to be
		 *   -rw------- if the file is created by this 
		 *   execution of nohup.
		 *
		 *   So, set up umask to do the right thing.
		 */
		flag=~(S_IRUSR|S_IWUSR);
	        oldmask=umask(flag);

		if( (temp = fopen(nout, "a")) == NULL) {
			if((home=getenv("HOME")) == NULL) {
				fputs((catgets(catd,NL_SETN,102, "nohup: cannot get $HOME\n")), stderr);
				exit(127);
			}
			strcpy(nout,home);
			strcat(nout,"/nohup.out");
			if(freopen(nout, "a", stdout) == NULL) {
				fputs((catgets(catd,NL_SETN,103, "nohup: cannot open/create nohup.out\n")), stderr);
				exit(127);
			}
		}
		else {
			fclose(temp);
			freopen(nout, "a", stdout);
		}
		fputs((catgets(catd,NL_SETN,104, "Sending output to ")), stderr);
		fputs(nout, stderr);
		fputc('\n', stderr);
                (void)umask(oldmask);
        }	
	if(isatty(2)) {
		close(2);
		dup(1);
	}
	execvp(argv[1], &argv[1]);
	err = errno;

/* It failed, so print an error */
	freopen("/dev/tty", "w", stderr);
	fputs(argv[0], stderr); fputs(": ", stderr);
	fputs(argv[1], stderr); fputs(": ", stderr);
	/*
	 *  use strerror - in understand languages.
	 */
	fputs(strerror(err), stderr);
	fputc('\n', stderr);
	/*
	 *   XPG4 requires and exit of 127 if "the utility specified
	 *     by the UTILITY operand could not be found" and an exit
	 *     of 126 if "the utility was found but could not be invoked."
	 */
	if (err == ENOENT |err == ENOTDIR |err == ENAMETOOLONG |err == ELOOP)
	   exit(127);
	else
	   exit(126);
}
