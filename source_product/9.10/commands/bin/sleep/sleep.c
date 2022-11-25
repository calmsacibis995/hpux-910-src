static char *HPUX_ID = "@(#) $Revision: 70.3 $";
/*
 * sleep -- suspend execution for an interval
 *
 *              sleep time
 */

#include <stdio.h>
#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

main(argc, argv)
int argc;
char **argv;
{

    int c;
    char *s;
    unsigned int n = 0;
    extern int optind;

#ifdef NLS
    nl_catd nlmsg_fd;
    nlmsg_fd = catopen ("sleep", 0);
#endif NLS

    while ((c=getopt(argc,argv,""))!=EOF);

    if (argc-optind != 1)
    {
        fputs((catgets(nlmsg_fd,NL_SETN,1, "usage: sleep time\n")), stderr);
        return 2;
    }

    s = argv[optind];
    while (c = *s++)
    {
        unsigned int prev_n = n;

        if (c < '0' || c > '9')
        {
            fputs((catgets(nlmsg_fd,NL_SETN,2, "sleep: bad character in argument\n")), stderr);
            return 2;
        }

        /*
         * Compute the new value of n.  If it is less than the
         * last value we had, we must have hit an overflow
         * condition.
         */
        if ((n = n * 10 + (c - '0')) < prev_n)
        {
            fputs((catgets(nlmsg_fd,NL_SETN,3, "sleep: illegal argument\n")), stderr);
            return 2;
        }
    }

#ifdef NLS
    catclose(nlmsg_fd);
#endif NLS
    sleep(n);
    return 0;
}

