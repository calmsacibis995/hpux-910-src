/*  $Revision: 64.1 $  */

/*  Program for making fonts  */

#include <stdio.h>
#include <fcntl.h>

#include FONTSOURCE

main()
{
	int size,fd;

	size = sizeof( bitdata );
	if ( (fd = creat(NAME,0444)) == -1) {
		fprintf(stderr,"cannot create font file\n");
		exit(1);
	}

	if ( write(fd,bitdata,sizeof(bitdata)) != size) {
		fprintf(stderr,"cannot write font file\n");
		exit(1);
	}

	exit(0);
}

