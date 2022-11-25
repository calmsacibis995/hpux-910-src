static char *HPUX_ID = "@(#) $Revision: 64.1 $";
/*
 *	lock.c
 *
 *	Lock a terminal up until someone types in a "key" password.
 */

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <sgtty.h>

struct	sgttyb	tty;
char	s1[BUFSIZ], s2[BUFSIZ];

main(argc, argv)
	char **argv;
{
	char *getpass();

	/*  set up lock to ignore interrupt, quit and CLD signals  */
	signal(SIGINT,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGCLD,  SIG_IGN);
#ifdef	SIGTSTP
	signal(SIGTSTP,  SIG_IGN);
#endif	SIGTSTP

	if (argc > 0) argv[0] = 0;

	/*  if there is no terminal attached, commit suicide  */
	if ( ioctl(0,TIOCGETP,&tty) ) exit(1);

	strcpy( s1, getpass("Key:   ") );
	strcpy( s2, getpass("Again: ") );
	
	if ( strcmp(s1,s2) ) exit(1);
	
	fputs("LOCKED\n", stdout);

	while( strcmp(getpass(""),s1) ) putchar(07);
	
}
