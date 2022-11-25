static char *HPUX_ID = "@(#) $Revision: 66.2 $";

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <cluster.h>

main(argc, argv)
int argc;
char *argv[];
{
    struct cct_entry *cct;
    struct cct_entry cctbuf;
    char *basename = strrchr(argv[0], '/');

    if (basename == (char *)0)
	basename = argv[0];
    else
	basename++;

    (void)setpgrp();

    /*
     * Find the rootserver entry from /etc/clusterconf
     */
    while ((cct = getccent()) != (struct cct_entry *)0)
    {
	if (cct->cnode_type == 'r')
	{
	    /*
	     * Make a copy of this entry, endccent() might free() it.
	     */
	    cctbuf = *cct;
	    cct = &cctbuf;
	    break;
	}
    }
    endccent();

    if (cct == (struct cct_entry *)0)
    {
	fprintf(stderr, "%s: No rootserver in /etc/clusterconf!\n",
	    basename);
	return 1;
    }

    if (cluster((MAX_CNODE+1) + 1, 0, cct->machine_id) == 0)
    {
	puts("Root server clustered.");
	return 0;
    }
    else
    {
	extern char *strrchr();
	char *errstring;

	switch (errno)
	{
	case EINPROGRESS:
	    errstring = "system already clustered";
	    break;
	case EAGAIN:
	    errstring = "cannot create limited CSP";
	    break;
	case ENXIO:
	    errstring =
		"LAN hardware is inconsistent with /etc/clusterconf";
	    break;
	case ENODEV:
	    errstring = "kernel not configured with dskless";
	    break;
	case EPERM:
	    errstring = "must be super-user";
	    break;
	case EIO:
	    errstring = "LAN hardware error";
	    break;
	default:
	    perror(basename);
	    return errno;
	}
	fprintf(stderr, "%s: %s\n", basename, errstring);
	return errno;
    }
}
