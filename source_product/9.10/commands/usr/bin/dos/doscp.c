/* @(#) $Revision: 64.2 $ */    
/******************************************************************************/
/*	Copies files from/to DOS format discs.  
	Issues: 
		does not copy file modes, but uses a default mode instead
*/
/******************************************************************************/


#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "dos.h"


#define flip(x)	 (x) = !(x)	

char	*malloc (), *strcat (), strncpy (), strcpy (), *strchr (), *strrchr ();
int 	stat();

#ifdef DEBUG
extern	boolean	debugon;
#endif DEBUG

extern char	*pname;
int	errcnt = 0;
int	verbose = FALSE;
int	force = FALSE;
boolean	uflg;

/* ----------- */
/* doscp_main  */
/* ----------- */
doscp_main (argc, argv)
int	argc;
char	**argv;
{
	struct	stat	bufr; /* needed for stat check of dest file */
	int	i;
	char 	go = 'n'; /* make user decide whether to continue */
			  /* if destination is a dosfile */

	pname = *argv++;
	argc--;
	
	/* process program arguments */
	uflg = FALSE;
	if (*argv!=NULL && **argv=='-' && (*((*argv)+1) != '\0')) {
		register char *s;

		argc--;
		for (s = *argv++ + 1; *s; s++) {
			switch (*s) {
			    case 'v' : flip (verbose);	break;
			    case 'f' : flip (force);	break;
			    case 'u' : flip (uflg);	break;
			    default : doscp_usage();
			}
		}
	}

	/* should have at least two parameters */
	if (argc < 2) 
		doscp_usage ();
	/* 
	* If the -f (force option) is used then: 
	* Check to see if user  realizes DOS files will 
	*  be written  over if the ":"  is forgotten from 
	*  the  destination  name. Checks for file existence and 
        *  gives the user a chance to exit from here.  
	*/

	if(!force){
	  if((strrchr(argv[argc - 1],':')) == NULL && 
			(strcmp(argv[argc -1],"-") != 0)) {

	   if( ((stat(argv[argc -1], &bufr)) == 0 ) 
		&& ( (bufr.st_mode & S_IFMT ) != S_IFDIR )) {
  	        fprintf (stderr,"Warning! %s will be overwritten. \n",
			argv[argc -1]);
	        fprintf (stderr,"Continue (y/n)? n \n"); 		
	        fscanf (stdin,"%c",&go);	
		 if(go != 'y') {
  	        	fprintf (stderr,"doscp: aborted\n");
			exit(1);
		 	}
		}
           }
	}


	/* ---------------------------------------------------------------- */
	/* find the destination file/directory.  If there are more than two */
	/* files specified, then the last file must be a directory.         */
	/* ---------------------------------------------------------------- */
	if (argc > 2) {

		char	name[MAXPATHLEN];
		int	i;

		/* dont allow the null string for a destination  */
		if (strlen(argv[argc-1]) <= 0){
			fprintf (stderr, "Improper(NULL) destination name \n");
			exit (1);
			}

		if (uflg == FALSE) 
			stoupper (argv[argc-1]);
		for (i=0; i<argc-1; i++) {

			/* check the length of the destnation  file name */
			if ((  strlen(argv[argc-1]) 
			     + strlen(dostail(argv[i]))) >= MAXPATHLEN-2){
			  fprintf (stderr, "Destination file name too long.  ");
			  fprintf (stderr, "Copy aborted for:");
			  fprintf (stderr, " %s/%s\n", argv[argc-1], argv[i]);
			  continue;
			  }

			strcpy (name, argv[argc-1]);
			strcat (name, "/");
			strcat (name, dostail (argv[i]));
			/* Convert DOS file names to upper case.  Note that a 
			   ':' must be found in the file name (must be a DOS 
			   file) before conversion takes place.   UNIX file
			   names are converted to lower case.
			*/
			if (uflg == FALSE) {
				sconvert (name);
				stoupper (argv[i]);
			}
			if (verbose)
				fprintf (stdout, "%s\n", argv[i]);
			errcnt += doscp (argv[i], name);
			}
		} 
	else  {
		/* Convert file names to upper case.  Note that a ':' 
		   must be found in the file name (must be a DOS file) 
		   before conversion takes place. 
		*/
		if (uflg == FALSE) {
			stoupper (argv[0]);
			stoupper (argv[1]);
		}
		if (verbose)
			fprintf (stdout, "%s\n", argv[0]);
		errcnt += doscp (argv[0], argv[1]);
		}

	mif_quit();
	exit (errcnt? 1 : 0);
}

/************/
/* SCONVERT */
/************/
/* Convert the last element of the file name to the proper case.  Assumes
   that at least one '/' is in the name.
*/
sconvert (name)
char	*name;
{
	char	*p;
	char	*s;
		
	if ((s = strchr (name, ':')) == NULL) {
		p = strrchr (name, '/') + 1;
		while (*p != '\0') {
			*p = tolower (*p);
			p++;
		}
	} else {	/* MUST BE A DOS NAME */
		p = strrchr (s+1, '/') + 1;
		while (*p != '\0') {
			*p = toupper (*p);
			p++;
		}
	}
}


#define BUFLEN	512

/* --------------------------- */
/* doscp - copy DOS/HPUX files */
/* --------------------------- */
doscp (source, dest)
char	*source;
char	*dest;
{
	int	dp;
	int	sp;
	int	rlen;
	int	dlen;
	char	buf[BUFLEN];

	/* setup source file descripter.  If file name is - then use 
	   standard in for source data */
	if (strcmp(source, "-") == 0) {
	  sp = -1;
	 } else {
	  sp = dosopen (source, O_RDONLY);

	  if (sp == -1) {
  	    fprintf (stderr,"%s: Could not open %s for reading\n", 
		     pname, source);
	    return (-1); 
	    } 
	  if (is_directory (sp)){
  	    fprintf(stderr,"%s: Cannot copy a directory,\n %s is a directory\n",
		     pname, source);
	    return (-1); 
	    } 
	  }

	/* setup destination file descripter.  If file name is - then use 
	   standard out for destination data */
	if (strcmp(dest, "-") == 0) {
	  dp = -1;
	 } else {
	  dp = dosopen (dest, O_RDWR|O_CREAT|O_TRUNC); 
	  if (is_directory (dp)) {
		char	name[MAXPATHLEN];
		/* dont allow the null string for a destination  */
		dosclose (dp);
		if (strlen(dest) <= 0){
			fprintf (stderr, "Improper(NULL) destination name \n");
			exit (1);
			}

		/* check the length of the destination  file name */
		if ((strlen(dest) + strlen(dostail(source))) >= MAXPATHLEN-2){
		  fprintf (stderr, "Destination file name too long.  ");
		  fprintf (stderr, "Copy aborted for:");
		  fprintf (stderr, " %s\n", source);
		  exit (1);
		  }
		strcpy (name, dest);
		strcat (name, "/");
		strcat (name, dostail (source));
		if (uflg == FALSE) 
			sconvert (name);
	  	dp = dosopen (name, O_RDWR|O_CREAT|O_TRUNC); 
	  }
	  if (dp == -1) { 
		fprintf (stderr,"%s: Could not create %s\n", pname, dest); 
		dosclose (sp);
		return (-1);
		}
	  }

	for (;;) {
		if (sp == -1)
			rlen = stdinread (buf, BUFLEN);
		else    rlen = dosread (sp, buf, BUFLEN);

		if (rlen <= 0)
			break;
		if (dp == -1)
			dlen = stdoutwrite (buf, rlen);
		else    dlen = doswrite (dp, buf, rlen);
		if (dlen != rlen) {
		  fprintf (stderr,"%s: Data write error to %s\n",pname,dest);
		  if (dp != -1) dosclose (dp);
		  if (sp != -1) dosclose (sp);
		  return (-1);
		}
	}
	if (dp != -1) dosclose (dp);
	if (sp != -1) dosclose (sp);
	return (0);
}

/* --------------------------------------------- */
/* doscp_usage					 */
/* --------------------------------------------- */
doscp_usage()
{
	fprintf (stderr, "Usage: %s [-fvu] f1 f2\n", pname);
	fprintf (stderr, "       %s [-fvu] f1 ... fn d1\n", pname);
	fprintf (stderr, "       %s [-fvu] f1 -\n", pname);
	fprintf (stderr, "       %s [-fvu] -  f2\n", pname);
	exit(2);
}

	
/* --------------------------------------------- */
/* stdoutwrite					 */
/* --------------------------------------------- */
stdoutwrite (buf, len)
char	*buf;
int	len;
{
	int	ret;

	ret = len;
	while (len > 0) {
		len--;
		if (putchar (*buf++) == EOF)
			return (ret - len);
		}
	return (ret);
}



/* --------------------------------------------- */
/* stdinread					 */
/* --------------------------------------------- */
stdinread (buf, len)
char	*buf;
int	len;
{
	static 	int	seeneof = 0;
	int 	i;
	int	c;

	if (seeneof) return (0);

	i = 0;
	while ((i<len) && !(seeneof = ((c = getchar()) == EOF))) {
		i++;
		*buf++ = c;
		}
	return (i);
}

