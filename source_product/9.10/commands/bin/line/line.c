static char *HPUX_ID = "@(#) $Revision: 56.1 $";
/*
	This program reads a single line from the standard input
	and writes it on the standard output. It is probably most useful
	in conjunction with the Bourne shell.
*/
#include <sys/signal.h>
#define LSIZE 512
int catch();
int EOF;
char nl = '\n';
main(argc, argv)
int argc;
char *argv[];
{
	register char c;
	char line[LSIZE];
	register char *linep, *linend;

EOF = 0;
linep = line;
linend = line + LSIZE;

if (argc > 2 && strcmp(argv[1], "-t") == 0) {
    int x = atoi(argv[2]);
    signal(SIGALRM, catch);
    alarm(x);
}

while ((c = readc()) != nl)
	{
	if (linep == linend)
		{
		write (1, line, LSIZE);
		linep = line;
		}
	*linep++ = c;
	}
alarm(0);
write (1, line, linep-line);
write(1,&nl,1);
if (EOF == 1) exit(1);
exit (0);
}
readc()
{
	char c;
if (read (0, &c, 1) != 1) {
	EOF = 1;
	return(nl);
	}
else
	return (c);
}


catch()
{
    exit(1);
}

