static char    *HPUX_ID = "@(#) $Revision: 66.1 $";

/****************
 * 
 *  Parseman is used by mkwhatis and fixman to get the list of directories
 *  defined by MANPATH.
 *  The directories are parsed out by the same functions called by man.c
 *  and catman.c and returned as a space-separated list of pathnames.
 *
 ***************/
#include <stdio.h>

#ifndef PATH_MAX
#  define PATH_MAX	1023
#endif

/*
 * next_dir() is in manlib.c.
 */
extern unsigned char *next_dir();
unsigned char Dot[PATH_MAX + 1];

main(argc,argv)
int argc;
unsigned char *argv[];
{
	unsigned char *manpath;
	unsigned char *loc;


	/*
	 *  argv[1] is the string, $MANPATH
	 */
	manpath = argv[1];

        /*
	 *  Get value of current directory to use in case of '.' in MANPATH
   	 */
	if(getcwd(Dot,PATH_MAX + 1) == NULL){
	    fprintf(stderr,"Can't get current directory.\n");
	    exit(1);
	}

	/*
	 *  next_dir() returns each pathname in turn until it runs off
	 *  the end.  Each one is output followed by a space.
	 */
        while((loc = next_dir(&manpath)) != NULL)
	    printf("%s ", loc);
	/*
	 *  Terminate all names with a newline.
	 */
	printf("\n");
	exit(0);
}
