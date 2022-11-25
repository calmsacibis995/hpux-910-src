/* @(#) $Revision: 66.1 $ */     
/*
 *	acctdisk
 *
 *	Acctdisk reads lines that contain user ID, login name, and 
 *	number of disk blocks from the standard input and converts them
 *	to total accounting records that can be merged with other 
 *	accounting records which are written to the standard output.
 *
 */

#include <sys/types.h>
#include "acctdef.h"
#include <stdio.h>
#include "tacct.h"

struct	tacct	tb;
char	ntmp[NSZ+1];

main(argc, argv)
char **argv;
{
	tb.ta_dc = 1;
	while(scanf("%ld\t%s\t%f",&tb.ta_uid,ntmp,&tb.ta_du) != EOF) {
		CPYN(tb.ta_name, ntmp);
		fwrite(&tb, sizeof(tb), 1, stdout);
	}
}
