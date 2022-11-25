#ifndef lint
static char rcsid[] = "@(#)portmon:	$Revision: 1.14.109.1 $	$Date: 91/11/19 14:10:58 $  ";
#endif
/* 
 * portmon: toggle the status of the portmonitoring
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

#define X_NFS_PORTMON 0
struct nlist nl[] = {
#ifdef hp9000s800
	{ "nfs_portmon" },
#else hp9000s800
	{ "_nfs_portmon" },
#endif hp9000s800
	"",
};

int kmem;			/* file descriptor for /dev/kmem */

char *vmunix = "/hp-ux";	/* HPNFS /vmunix is SUN's equivalent to */
				/* HPNFS our /hp-ux			*/

char *core = "/dev/kmem";	/* name for /dev/kmem */


int nfs_portmon;		/* Place to strore the value in the kernel */

#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

main(argc, argv)
	int argc;
	char *argv[];
{
	int onflag = 0;
	int offflag = 0;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("nfsstat",0);
#endif NLS
	
	/* Since usage is "portmon [on | off ] [/hp-ux]", don't allow more
	 * than 3 arguments
	 */
	if (  argc > 3 )
		usage();
	while (argc > 1) {
		if ( !strcmp(argv[1],"on") ) {
			if ( offflag )
				usage();
		        onflag++;
		}
		else if ( ! strcmp(argv[1],"off") ) {
			if ( onflag )
				usage();
			offflag++;
		}
		else {	/* last argument must be the name of the kernel */
		    if ( argc > 2 )
			usage();
		    vmunix = argv[1];
		}
		argc--;
		argv++;
	}

	setup(onflag|offflag);
	getinfo();
	if (onflag && !nfs_portmon ) { /* we want it on and its not */
		nfs_portmon = 1;
		putinfo();
	}
	else if (offflag && nfs_portmon ) {
		nfs_portmon = 0;
		putinfo();
	}

	printf((catgets(nlmsg_fd,NL_SETN,1, "NFS port monitoring is ")));
	if ( nfs_portmon )
	    printf((catgets(nlmsg_fd,NL_SETN,2, "ON\n")));
	else
	    printf((catgets(nlmsg_fd,NL_SETN,3, "OFF\n")));

	exit (0);
}

getinfo()
{
	if (lseek(kmem, (long)nl[X_NFS_PORTMON].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "portmon: can't seek in kmem\n")));
		exit(1);
	}
	if (read(kmem, &nfs_portmon, sizeof(int)) != sizeof(int)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "portmon: can't read rcstat from kmem\n")));
		exit(1);
	}

}

putinfo()
{
	if (lseek(kmem, (long)nl[X_NFS_PORTMON].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "portmon: can't seek in kmem\n")));
		exit(1);
	}
	if (write(kmem, &nfs_portmon, sizeof(int)) != sizeof(int)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "portmon: can't write nfs_portmon to kmem\n")));
		exit(1);
	}
}

setup(changeflag)
	int changeflag;
{
	register struct nlist *nlp;

	nlist(vmunix, nl);
	if (nl[0].n_value == 0) {
		fprintf (stderr, (catgets(nlmsg_fd,NL_SETN,7, "portmon: Unable to get variable list from %s\n")),vmunix);
		usage();
	}
	if ((kmem = open(core, changeflag ? 2 : 0)) < 0) {
		perror(core);
		exit(1);
	}
}

usage()
{
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "Usage: portmon [ on | off ] [namelist]\n")));
	exit(1);
}
