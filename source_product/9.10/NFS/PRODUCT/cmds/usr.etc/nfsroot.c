#ifndef lint
static char rcsid[] = "@(#)nfsroot:	$Revision: 1.13.109.1 $	$Date: 91/11/19 14:10:46 $  ";
#endif
/* 
 * nfsroot: query/change the value of "nobody" for nfs.
 *
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
char *catgets();
#endif NLS

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <nlist.h>

#define X_NOBODY 0
struct nlist nl[] = {
#ifdef hp9000s800
	{ "nobody" },
#else hp9000s800
	{ "_nobody" },
#endif hp9000s800
	"",
};

int kmem;			/* file descriptor for /dev/kmem */

char *vmunix = "/hp-ux";	/* HPNFS /vmunix is SUN's equivalent to */
				/* HPNFS our /hp-ux			*/

char *core = "/dev/kmem";	/* name for /dev/kmem */


long nfs_nobody;		/* Place to strore the value in the kernel */

#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

main(argc, argv)
	int argc;
	char *argv[];
{
	int newvalflag = 0;
	long newval;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("nfsstat",0);
#endif NLS
	
	/* Since usage is "nobody [<n>] [/hp-ux]", don't allow more
	 * than 3 arguments
	 */
	if (  argc > 3 )
		usage();
	while (argc > 1) {
		/*
		 * is it a number?  If it doesn't start with a digit (or minus)
		 * and the rest consist of only digits, then it's not a number
		 */
		if ( strchr("-0123456789",argv[1][1]) != NULL &&
		    strspn(&argv[1][1],"0123456789") == strlen(&argv[1][1])) {
		    newval = atoi(argv[1]);
		    newvalflag++;
		}
		else {	/* last argument must be the name of the kernel */
		    if ( argc > 2 )
			usage();
		    vmunix = argv[1];
		}
		argc--;
		argv++;
	}

	setup(newvalflag);
	getinfo();

	/* Check to see if we need to change the value? */
	if ( newvalflag && nfs_nobody != newval ) {
		nfs_nobody = newval;
		putinfo();
	}

	printf((catgets(nlmsg_fd,NL_SETN,3, "nobody = %d\n")), nfs_nobody);

	exit (0);
}

getinfo()
{
	if (lseek(kmem, (long)nl[X_NOBODY].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "nfsroot: can't seek in kmem\n")));
		exit(1);
	}
	if (read(kmem, &nfs_nobody, sizeof(long)) != sizeof(long)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "nfsroot: can't read nobody from kmem\n")));
		exit(1);
	}

}

putinfo()
{
	if (lseek(kmem, (long)nl[X_NOBODY].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "nfsroot: can't seek in kmem\n")));
		exit(1);
	}
	if (write(kmem, &nfs_nobody, sizeof(long)) != sizeof(long)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "nfsroot: can't write new nobody value to kmem\n")));
		exit(1);
	}
}

setup(changeflag)
	int changeflag;
{
	register struct nlist *nlp;

	nlist(vmunix, nl);
	if (nl[0].n_value == 0) {
		fprintf (stderr, (catgets(nlmsg_fd,NL_SETN,7, "nfsroot: Unable to get variable list from %s\n")), vmunix);
		usage();
	}
	if ((kmem = open(core, changeflag ? 2 : 0)) < 0) {
		perror(core);
		exit(1);
	}
}

usage()
{
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "Usage: nfsroot [<n>] [namelist]\n")));
	exit(1);
}
