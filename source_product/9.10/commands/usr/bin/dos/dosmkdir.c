/* @(#) $Revision: 63.1 $ */     
/******************************************************************************/
/*	This program makes a MSDOS directory
		Issues:	-must allocate at least one cluster so that .. can work
			-are . and .. only supported on some discs?
			-cleanup in case of accident
*/
/******************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include "dos.h"

extern char	*strchr(), *strrchr();
#define flip(x)	 (x) = !(x)	

extern int	errcode;

#ifdef DEBUG
extern	boolean	debugon;
#endif DEBUG

extern char	*pname;

/* -------------- */
/* dosmkdir_main  */
/* -------------- */
dosmkdir_main (argc, argv)
int	argc;
char	**argv;
{

	boolean	uflg;	/* TRUE means don't convert file name to upper case */

	pname = *argv++;
	argc--;
	errcode = 0;

	/* process program arguments */
	uflg = FALSE;
	while (*argv!=NULL && **argv=='-') {
		register char *s;
		argc--;

		for (s = *argv++ + 1; *s; s++) {
			switch (*s) {
			    case 'u': flip (uflg);
				      break;
			    default : goto usage;
			}
		}
	}

	if (argc <= 0) goto usage;

	while (argc--){
		if (uflg == FALSE) stoupper (*argv);
		dosmkdir (*argv);
		++argv;
	}

	mif_quit();
	exit (errcode? 2:0);

usage:
	/* UPPERCASE	Describe uppercase conversion? */
	fprintf (stderr, "usage: %s [-u] device:file ... \n", pname );
	exit(1);
}


/* --------------------------------- */
/* dosmkdir -  make MSDOS dirctories */
/* --------------------------------- */
dosmkdir (p)
char	*p;
{
	int	dp;
	int	sp;
	struct	info		*spi;
	struct	info		*dpi;
	struct	dir_entry	dir;
	char 			*s, *d, *t;
	char	dirname[MAXPATHLEN];

	/* open the parent directory */
	t = dostail (p);
	for (s=p, d=dirname; s!=t;  *d++ = *s++);
	*d = '\0';

	sp = dosopen (dirname, O_RDWR);
	if (sp==-1) {
		fprintf (stderr, "%s: Can't open parent directory for %s\n",
				pname, p);
		errcode++;
		return (-1);
	}

	/* must be a MSDOS format device */
	if (!dosfile(sp)) {
		fprintf (stderr, "%s: %s not a MSDOS file\n", pname, p);
		errcode++;
		return (-1);
	}

	spi = CFD(sp);

	/* create a standard file and turn it into a directory */
	dp = dosopen (p, O_RDWR|O_CREAT|O_EXCL);
	if (dp == -1) {
		dosclose (sp);
		fprintf (stderr, "%s: Can't create %s\n", pname, p);
		errcode++;
		return (-1);
	}
	dpi = CFD(dp);
	dpi->dir.attr = ATTDIR;
	if (update_dir_entry (dpi) == -1) {
		errcode++;
		goto cleanup;
		}
	

	/* note that . needs to be the first entry in the directory and
	   .. has to be the second entry in the directory.  The write
	   directory entry code automatically fills newly allocated 
	   disc space with unused entries 
	*/
	/* create an entry for  .. */
	dir = dpi->dir;
	dir.cls = spi->dir.cls;
	strcpy (dir.name,  "..      ");
	strcpy (dir.ext,  "   ");
	if (write_dir_entry (dpi, 1, &dir) != DIRSIZE) {
	  fprintf (stderr,"%s: Can't create .. in %s\n",pname,p);
	  errcode++;
	  goto cleanup;
	}

	/* create an entry for . */
	dir.cls = dpi->dir.cls;
	strcpy (dir.name,  ".       ");
	if (write_dir_entry (dpi, 0, &dir) != DIRSIZE) {
	  fprintf (stderr,"%s: Can't create . in %s\n",pname,p);
	  errcode++;
	  goto cleanup;
	}


cleanup:
	dosclose (dp);
	dosclose (sp);
	return (0);
}

