static char *HPUX_ID = "@(#) $Revision: 66.1 $";

#include <stdio.h>
#include <sys/param.h>

main(argc,argv)
int argc;
char *argv[];
{
    static char *usage = "Usage: showcdf [-chs] names\n";
    extern char *getcdf();
    extern char *hidecdf();
    int mute = 0;
    int quiet = 0;
    int hide = 0;
    int err = 0;
    int count = 0;
    int i;
    char *cptr;
    char buf[MAXPATHLEN];

    while ((argc > 1) && (argv[1][0] == '-'))
    {
	cptr = &argv[1][1];
	while (*cptr)
	{
	    switch (*cptr)
	    {
	    case 'c': 
		quiet = 1;
		break;
	    case 'h':
		hide = 1;
		break;
	    case 's': 
		mute = 1;
		break;
	    default: 
		fputs(usage, stderr);
		exit(2);
		break;
	    }
	    cptr++;
	}
	argc--;
	argv++;
    }

    if (argc < 2)
    {
	fputs(usage, stderr);
	exit(2);
    }

    for (i = 1; i < argc; i++)
    {
	int same;
	cptr = (*(hide ? hidecdf : getcdf))(argv[i], buf, MAXPATHLEN);
	if (cptr == (char *)NULL)
	{
	    err = 1;
	    if (!mute)
	    {
		fputs(argv[i], stderr);
		fputs(": Inaccessible\n", stderr);
	    }
	    continue;
	}

	same = strcmp(argv[i], cptr) == 0;
	if (!same)
	    count++;

	if (!mute && !(quiet && same))
	{
	    fputs(cptr, stdout);
	    fputc('\n', stdout);
	}
    }

    if (err)
	exit(2);
    if (count)
	exit(1);
    exit(0);
}
