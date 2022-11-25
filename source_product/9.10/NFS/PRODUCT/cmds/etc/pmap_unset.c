/*	@(#)$Revision: 1.11.109.1 $	$Date: 91/11/19 14:02:05 $
**	<pmap_unset.c>	--	front-end to pmap_unset()
*/

#include <rpc/rpc.h>

extern	char	*optarg;
extern	int	optind, opterr;

main(argc,argv)
int argc;
char **argv;
{
    int prog, vers = 1;

    for (--argc, ++argv; argc > 0; --argc, ++argv) {
	if ((*argv)[0] == '-' && (*argv)[1] == 'v') {
	    vers = atoi(*++argv);
	    --argc;
	    continue;
	} else {
	    prog = atoi(*argv);
	}
	printf("pmap_unset(prog=%d, vers=%d)\n", prog, vers);
	(void) pmap_unset(prog, vers);
    }
    /*
    **	after we're all done, execl rpcinfo to look at the table
    */
    execl("/usr/etc/rpcinfo", "rpcinfo", "-p", 0);
}
