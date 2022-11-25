static char *HPUX_ID = "@(#) $Revision: 51.1 $";

/*
 * Setprivgrp -- make the named group have privileges named
 *		 for special kernel access.
 * usage:
 *      setprivgrp -n|-g|group-name <privilege name>*
 *   or
 *	setprivgrp -f <file name>
 *
 * If the -f option is used, the format of the contents of <file name> is:
 *      -n|-g|group-name <privilege name>*
 *
 * If option -g is used, the privilege is granted globally.
 * If option -n is used, the privileges are taken away from every
 * privileged group currently in the system.
 */

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <grp.h>
#include <signal.h>
#include <sys/privgrp.h>
#include <errno.h>
#include "privnames.h"

#define MAXNAMELEN 256
#define NPRIVS (sizeof(privnames)/sizeof(privnames[0]))
#define PRIVNAMELEN 16
#define register /**/

main(argc, argv)
    int argc;
    char * argv[];
{
    register int i;
    char group_name[MAXNAMELEN];
    static char *privilege_names[NPRIVS + 1];
    static char namespace[NPRIVS + 1][PRIVNAMELEN];
    register FILE *fromfile;
    int sigsys();

    signal(SIGSYS, sigsys);

    /* initialize space for privilege_names. This is done in this way */
    /* so that the space will be varied according to NPRIVS.          */

    for (i = 0; i < NPRIVS + 1; i++)
	privilege_names[i] = &namespace[i][0];

    if (argc == 1) {
	usage();
	exit(0);
    }

    if (argv[1][0] == '-')
	switch(argv[1][1]) {

	case 'f':
	    if (argc != 3) {
		usage();
		exit(5);
	    }
	    if ((fromfile = fopen(argv[2], "r")) == (FILE *)0) {
		printf("Cannot open file %s\n",argv[2]);
		usage();
		exit(5);
	    }
	    while (gettokline(fromfile, group_name, privilege_names)) {
		/*printf( " privilegename %s %s %s %s \n",privilege_names[0],
		privilege_names[1],privilege_names[2],privilege_names[3]);*/

		if (**privilege_names == '\0' && !strcmp(group_name,"-n")) {

			/*
			 * Revoke all privileges
			 */
			for (i = 0; i < NPRIVS; i++)
			    strcpy(privilege_names[i], privnames[i].name);
			privilege_names[NPRIVS][0] = '\0';
		}
		do_setprivgrp(group_name, privilege_names);
	    }
	    exit(0);
	case 'n':
	    /*
	     * -n is to revoke named privilege.
	     */
	    if (argc == 2) {
		/*
		 * Revoke all privileges
		 */
		for (i = 0; i < NPRIVS; i++)
		    strcpy(privilege_names[i], privnames[i].name);
		privilege_names[NPRIVS][0] = '\0';
		do_setprivgrp(argv[1], privilege_names);
		exit(0);
	    }
	    break;

	case 'g':
	    break;

	default:
	    printf("unknown option %s\n",argv[1]);
	    usage();
	    exit(5);
	
	}
    
    /*
     * call setprivgrp with command line argument
     */
    do_setprivgrp(argv[1], &argv[2]);
    exit(0);
}

usage()
{
    register int i;

    printf("usage: setprivgrp -n|-g|group-name ");
    for (i = 0; i < NPRIVS; i++)
	printf("[%s] ", privnames[i].name);
    printf("\n    or setprivgrp -f file-name\n");
}

do_setprivgrp(group, priv_list)
    char *group, *priv_list[];
{
    register int group_id, i, j;
    struct group *grent, *getgrnam();
    int mask[PRIV_MASKSIZ];
    extern int errno;

    for (i = 0; i < PRIV_MASKSIZ; i++)
	mask[i] = 0;

    /*
     * see if group asked for was "-g", if so, grant global access for
     * privileges. If group asked for was "-n" revoke privileges.
     */

    if (!strcmp(group, "-g"))
	group_id = PRIV_GLOBAL;
    else if (!strcmp(group, "-n"))
	group_id = PRIV_NONE;
    else {
	grent = getgrnam(group);
	if (grent == (struct group *)0) {
	    fprintf(stderr,"setprivgrp: no group name matching %s\n",group);
	    exit(3);
	} else
	    group_id = grent->gr_gid;

    }


    /*
     * Get privilege mask by associating priv_list entries with
     * entry numbers in the privnames array to yield the
     * privilege number.
     */
#define setmask(n) mask[(n-1)/(sizeof(int)*8)] |= 1<<((n-1) % (sizeof(int)*8));

     for (;*priv_list != NULL && **priv_list; priv_list++) {
	for (i = 0; i < NPRIVS; i++)
	    if (!strcmp(privnames[i].name, *priv_list)) {
/*	    printf(" NPRIVS,i,privenames,priv_list %d %o %s %s\n",
		NPRIVS,i,privnames[i].name, *priv_list); */
		setmask(privnames[i].number);
		break;
	    }
	if (i == NPRIVS) {
	    /*
	     * Couldn't find an privilege associated with *priv_list
	     */
	    printf("setprivgrp:no privilege associated with %s\n", *priv_list);
	    usage();
	    exit(4);
	}
    }

    /*
     * Do the setprivgrp system call
     */
    if (setprivgrp(group_id, mask)) {
	switch (errno) {
	case EPERM:
	    printf("setprivgrp: NOT super-user\n");
	    exit(1);
	case E2BIG:
	    printf("setprivgrp: out of privileged group slots\n");
	    exit(2);
	default:
	    perror("setprivgrp");
	    exit(5);
	}
    }
}



/*
 * Get a line of input as tokens and return in arguments
 */
gettokline(file, group, privlist)
    FILE *file;
    char *group, *privlist[];
{
    register int c, i, j;
    register char * s;

    /*   initialize the return array    */   

    for ( i = 0; i< NPRIVS + 1;i++)
	*privlist[i] = '\0';

    *group = '\0';

    /*  throw away leading blanks   */

    c = getc(file);
    while ( c == ' ' || c == '\t') c = getc(file);
    if( c == '\n' || c == '\0' ) return(1);
    if( c == EOF ) 
	return(0);
    else
	ungetc(c,file);

    /*  this is where we really begin to get the tokens */
    
    for (  s = group, c = getc(file), i = 0, j = 0
	 ; c != '\n' && c != '\0'
	 ; c = getc(file)) {
	 if (c == EOF) return(0);
	 if (isspace(c)) {
	    *s = '\0';
	    s = privlist[i++];
	    j = 0;
	    if( i > NPRIVS ) { 
		    while( c != '\n' && c !='\0') c=getc(file);
		    return(1);
		    }
	    while (c == ' ' || c == '\t') 
		c = getc(file);
    		if( c == '\n' || c == '\0' ) return(1);
		if( c == EOF )
			return(0);
		else
			/*  put last unblank char back  */
			ungetc(c,file);
	    }
	 else if (isalnum(c) || c == '-' || c == '.' || c == '~' || c == '/' || c == '_')
	    { 
	    if ( j < PRIVNAMELEN ) {
		*s++ = c;
		j++;
		}
            }
    }
    *s = '\0';
    
}

sigsys( )
{
	printf("setprivgrp:no system call for setprivgrp\n");
	exit(5);
}
/*
 * Get a line of input as tokens and return in arguments
 */
/*
gettokline(file, group, privlist)
    FILE *file;
    char *group, *privlist[][NPRIVS];
{
    register int c, i;
    register char * s;

    for (  s = group, c = getc(file), i = 0
	 ; c != '\n' && c != '\0'
	 ; c = getc(file)) {
	if (isalnum(c) || c == '.' || c == '~' || c == '/' || c == '_')
	    *s++ = c;
	else if (isspace(c)) {
	    *s = '\0';
	    s = privlist[i++];
	}
    }
    *s = '\0';
}
*/

/*
sigsys()
{
    printf("setprivgrp:no system call for setprivgrp\n");
    exit(5);
}
*/
