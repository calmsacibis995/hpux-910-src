/* @(#) $Revision: 38.1 $ */    
static char *HPUX_ID = "@(#) $Revision: 38.1 $";
#include "s500defs.h"
#include "sdf.h"
#include <stdio.h>
#include <ustat.h>
#include <sys/stat.h>
#include <fcntl.h>

char *pname;

#ifdef	DEBUG
int debug = 0;
#endif	DEBUG

main(argc, argv)
register int argc;
register char **argv;
{
	struct ustat ustbuf;

	pname = *argv;
	if (argc < 2)
		usage();
	while (--argc > 0) {
		if (**++argv == '-')
			usage();
		if(sdfustat(*argv, &ustbuf) < 0) {
			fprintf(stderr,"%s: cannot access %s\n", pname, *argv);
			continue;
		}
/* 
 * added to reflect Bell System V standardization of reporting disc blocks
 * in terms of 512-byte blocks (as opposed to physical blocksizes):
 */
		ustbuf.f_tfree = ustbuf.f_tfree * ustbuf.f_blksize / 512;
		ustbuf.f_tinode = ustbuf.f_tfree * 512 / FA_SIZ;
	
		printf("%-10s: %8ld blocks %11u i-nodes\n", *argv,
		    ustbuf.f_tfree, ustbuf.f_tinode);
	}
	exit(0);
}

usage()
{
	fprintf(stderr, "Usage: %s device:file [...]\n", pname);
	exit(1);
}
