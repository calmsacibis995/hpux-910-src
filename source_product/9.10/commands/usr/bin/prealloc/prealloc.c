static char *HPUX_ID = "@(#) $Revision: 64.1 $";
 
/*    
 *
 *	prealloc will preallocate an ordinary file of zero length given
 *	a file name and a byte size.  It will create the file if it does
 *	not already exitst.
 *
 *	usage: prealloc name size
 *
 *	prealloc exits with 0, upon successful completion.
 *  	                    1, if name already exists and not a ordinary 
 *	                       file of zero length.
 *	                    2, if there is not enough room on disc.
 *	                    3, if size exceeds file size limits.
 *
 * 
 *      author: B.C.
 *
 */	
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

extern int errno;

main(argc, argv)
    int argc;
    char * argv[];
{
    int fd;
    int size;
    char *ptr;    

    if (argc != 3) usage();

    size =  strtol(argv[2], &ptr, 10); 
    if ( size < 0  || (*ptr != NULL ) ) {
    	fputs("prealloc: invalid file size\n", stderr);
	exit(3);
    }
	
    /*  see if the file exists  */

	     
    
    /* 	try open the file for a write  */

    if ( (fd = open(argv[1],1) ) == -1 ) {

	/*   does not exist go ahead and creat it */

	if( (fd = creat(argv[1],0666 ) ) == -1 ) {
	     char tmpstr[80];

	     strcpy(tmpstr, "prealloc: ");
	     strcat(tmpstr, argv[1]);
	     perror(tmpstr);
	     exit(1);
	}

	/*   make the system call to preallocate the file */
	
	do_prealloc(fd, (unsigned) size, argv);


	
    }

    /* check if existing file is regular and zero length */

    else if (ordinary0( argv[1] ) ) { 

 		/*   make the system call to preallocate the file */

		do_prealloc(fd, (unsigned) size, argv);
		exit(0);
	 }
	 else {
		fputs("prealloc: ", stderr);
		fputs(argv[1], stderr);
		fputs(": File exists\n", stderr);
		exit(1);
	 }


}

usage()
{

    fputs("Usage: prealloc name size\n", stdout);
    exit(-1);
}

ordinary0(name)     /*   check if file is ordinary and zero length */
char *name;
{
	struct stat stbuf;

	if (stat(name,&stbuf) == -1) {
		perror("prealloc");
		return(1);
	}

	/*  see if file is regular and is length zero */

	if ( (stbuf.st_mode & S_IFMT) == S_IFREG) {
		if(stbuf.st_size == 0) return(1);
	}
	return(0);
}


/*	make the actual system call for prealloc   */

do_prealloc(fd,size,argv)
int fd;
unsigned size;
char * argv[];

{
	if ( prealloc(fd,size) ) {
		switch (errno) {
		case EBADF:
			perror("prealloc");
			close(fd);
			unlink(argv[1]);
			exit(1);
		case ENOTEMPTY:
			perror("prealloc");
			close(fd);
			unlink(argv[1]);
			exit(1);
		case ENOSPC:
			perror("prealloc");
			close(fd);
			unlink(argv[1]);
			exit(2);
		case EFBIG:
			perror("prealloc");
			close(fd);
			unlink(argv[1]);
			exit(3);
		default:
			perror("prealloc");
			close(fd);
			unlink(argv[1]);
			exit(-1);
		}

	}

}
