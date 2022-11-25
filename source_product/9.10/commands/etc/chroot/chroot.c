static char *HPUX_ID = "@(#) $Revision: 64.1 $";


# include <stdio.h>
/* chroot */

main(argc, argv)
char **argv;
{
	if(argc < 3) {
		fputs("usage: chroot rootdir command arg ...\n", stdout);
		exit(1);
	}
	argv[argc] = 0;

/*		get rid of absolute pointer stuff
	if(argv[argc-1] == (char *) -1) /* don't ask why */
/*		argv[argc-1] = (char *) -2;	*/

	if (chroot(argv[1]) < 0) {
		perror(argv[1]);
		exit(1);
	}
	if (chdir("/") < 0) {
		fputs("Can't chdir to new root\n", stdout);
		exit(1);
	}
	execv(argv[2], &argv[2]);
	close(2);
	open("/dev/tty", 1);
	fputs(argv[2], stdout);
	fputs(": not found\n",stdout);
	exit(1);
}
