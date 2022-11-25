static char *HPUX_ID = "@(#)$Revision: 51.2 $";
/************************************************************************
* NAME									*
*	uupath - translate path using pathalias routing 		*
* SYNPOSIS								*
*	uupath [ -f dbm_name ] address					*
* DESCRIPTION								*
*	The argument address is separated into host!rest-of-path	*
*	and the routing file is read for the first "host" line.  The	*
*	path is translated using the right hand side alias and ouput	*
*	to stdout.  If the host is not found, the original address is	*
*	output and an exit code of 1 is returned.			*
* 									*
* 	See manual page for rest of description - this one is outdated. *
************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <dbm.h>
#include <fcntl.h>

#ifndef FALSE
#define FALSE	0
#endif

#define	PATHFILE	"/usr/lib/mail/paths"

char	db_name[BUFSIZ];   /* data base pathname */
char	*malloc();
char 	buffer[BUFSIZ];	   /* buffer for filenames opened by dbmcheck */
int 	dbmcheck();

main(argc,argv)
int	argc;
char	**argv;
{
	register char *delim, *user, *host, *cp, *arg;
	datum hostname, format;
	
 	if (argc != 2 && argc != 4){
 		fprintf(stderr,"Usage: %s [ -f dbm_name ] address\n",*argv);
		exit(2);
	}

 	if (argc == 4 && strcmp(argv[1], "-f") == 0) {
 		strcpy(db_name, argv[2]);
 		argv++, argv++;
 	} else 
 		strcpy(db_name, PATHFILE);
 
	if (dbmcheck(db_name) < 0) {	/* check for access to db_name */
		fprintf(stderr, "uupath: ");
		perror(buffer);
		exit(2);
		}
 
 	dbminit(db_name);

	arg = malloc(strlen(*++argv)+1);
	strcpy(arg, *argv);
	if ((delim = strchr(arg, '!')) == NULL)
		if ((delim = strchr(arg, '@')) == NULL) {
			puts(*argv);
			exit(1);
		}
	if (*delim == '!') {
		*delim = '\0';
		host = arg;
		user = ++delim;
	} else {
		*delim = '\0';
		user = arg;
		host = ++delim;
	}
	for (cp = host; *cp; cp++)
	    if (isupper(*cp))
		*cp = tolower(*cp);
	hostname.dptr = host;
	hostname.dsize = strlen(host)+1;
	format = fetch(hostname);
	if (format.dptr != NULL) {
			printf(format.dptr, user);
			putchar('\n');
			exit(0);
	} else {
		printf("%s\n", *argv);
		exit(1);
	}
}


dbmcheck(filename)
char *filename;
/* check filename.dir and filename.pag for read access */
/* in case of error, leaves name of offending file in buffer */
{
	strcpy(buffer, filename);
	strcat(buffer, ".dir");
	if (access(buffer, 04) < 0)
		return (-1);
	strcpy(buffer, filename);
	strcat(buffer, ".pag");
	if (access(buffer, 04) < 0)
		return (-1);
	return (0);
}
