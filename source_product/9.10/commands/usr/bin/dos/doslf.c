/* @(#) $Revision: 63.1 $ */    
/*	list the fat
*/
#include <stdio.h>
#include <fcntl.h>
#include "dos.h"

#define flip(x)	 (x) = !(x)	

char	*malloc (), *strcat (), strncpy (), strcpy (), *strchr (), *strrchr ();
extern	boolean	debugon;

char	*pname;
int	errcode;

/* ------ */
/* main - */
/* ------ */
main (argc, argv)
int	argc;
char	**argv;
{

	char	*arg;
	int	n;
	struct	info	mp;

	setbuf (stdout, NULL);
	errcode = 0;
	debugon = FALSE;
	mif_init ();

	pname = *argv++;			/* remember the program name */
	open_device (*argv, O_RDONLY, &mp.header);
	for (n=0; n<mp.header.max_cls; n++)
		printf ("(%x)", get_fat_entry (&mp.header, n));

}


