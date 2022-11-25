static char *HPUX_ID = "@(#) $Revision: 66.2 $";
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <cluster.h>
#include <sys/nsp.h>
#include <errno.h>
#include <sys/param.h>
#include <machine/param.h>
#include <string.h>

extern char *ultoa();
void why_einval();

char *progname;

int
main(argc, argv)
int argc;
char *argv[];
{
    extern char *optarg;
    extern int optind;

    int numcsps;
    int my_site;
    int command;

    progname = strrchr(argv[0], '/');
    if (progname == (char *)0)
	progname = argv[0];
    else
	progname++;

    /*
     * First check if this machine is a member of a DUX cluster. If
     * not then no sense in going on.
     */
    if ((my_site = cnodeid()) == 0)
	return usage(1);

    /*
     * Next check if there was a valid command line argument for
     * the number of KCSPs to creat.
     */
    switch (getopt(argc, argv, "a:d:"))
    {
    case 'a':
	command = NSP_CMD_DELTA;
	numcsps = atoi(optarg);
	if (numcsps < 0)
	    return usage(3);
	break;
    case 'd':
	command = NSP_CMD_DELTA;
	numcsps = -atoi(optarg);
	if (numcsps > 0)
	    return usage(3);
	break;
    case EOF:
	command = NSP_CMD_ABS;
	if (optind == argc)
	{
	    struct cct_entry *pce;

	    /*
	     * Nothing on the command line, so default to the dux
	     * clusterconf file to try and get the default info.
	     */
	    if ((pce = getcccid(my_site)) == (struct cct_entry *)NULL)
	    {
		endccent();
		return usage(4);
	    }

	    numcsps = pce->kcsp;
	    endccent();
	    if (numcsps < 0)
		return usage(5);
	}
	else if (optind == argc - 1)
	{
	    numcsps = atoi(argv[optind]);
	    if (numcsps < 0)
		return usage(3);
	    ++optind;
	}
	break;

    default:
	return usage(2);
    }

    /*
     * Its easy to check for the lower boundry, but the maximum KCSPs
     * could be a configurable kernel parameter.  Just pass the info to
     * the csp() system call, and let it worry about the upper boundry.
     */
    if (csp(command, numcsps) == -1)
    {
	if (errno == EINVAL)
	    fprintf(stderr,
		"%s: Kernel not configured with diskless drivers\n",
		progname);
	else
	    perror(progname);
	return 1;
    }

    if (command == NSP_CMD_ABS)
    {
	fputs(ultoa(numcsps), stdout);
	fputc('\n', stdout);
    }
    return 0;
}

int
usage(error)
register int error;
{
    static char CLUSTERCONF[] = "/etc/clusterconf";

    if (error != 2)
    {
	fputs(progname, stderr);
	fputs(": ", stderr);
    }

    switch (error)
    {
    case 1:
	fputs("Not a cluster node\n", stderr);
	break;
    case 3:
	fputs("Invalid value for number of CSPs to invoke!\n",
		stderr);
	break;
    case 4:
	fputs("Cannot find specified machine in ", stderr);
	fputs(CLUSTERCONF, stderr);
	fputc('\n', stderr);
	break;
    case 5:
	fputs("Invalid value for number of CSPs to invoke in ",
		stderr);
	fputs(CLUSTERCONF, stderr);
	fputc('\n', stderr);
	break;
    case 2:
    default:
	fprintf(stderr, "Usage: %s [-rz] [-adn number_of_csps]\n",
	    progname);
	break;
    }
    return 1;
}
