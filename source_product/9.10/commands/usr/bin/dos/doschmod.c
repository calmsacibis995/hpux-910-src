/* @(#) $Revision: 63.2 $ */     
/******************************************************************************/
/* This program allows you to change the attributes byte on a
	MSDOS file.

	Issues:  should we allow the directory attribute to be changed?
		 resolved: no, invitation to disaster.

*/
/******************************************************************************/

#include <stdio.h>
#include "dos.h"
#include <fcntl.h>

#define	flip(x)	(x)=!(x)

#ifdef DEBUG
extern	boolean	debugon;
#endif DEBUG

extern int	errcode;	/* count of the number of errors which occur */
extern char	*pname;		/* program name */

/* -------------- */
/* doschmod_main  */
/* -------------- */
doschmod_main (argc, argv)
int	argc;
char	**argv;
{

	boolean		uflg;
	struct info	*mp;
	char		*p;
	int		mode;
	int		i;
	int		sp;

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

	if ((argc<=1) || *argv==NULL) {
usage:
	  fprintf (stderr, "usage: %s [-u] octal_mode device:file ... \n", pname );
	  exit(1);
	}

	/* determine what the attribute byte should be */
	if ((mode = octal (argv[0])) == -1) {
		fprintf (stderr, "%s: Attribute must be octal value (0-255)\n",
			pname);
		exit (1);
		}

	if (mode & 0x10) {
		fprintf (stderr, "%s: Not allowed to set the directory bit, use dosmkdir instead\n",
			pname);
		exit (1);
		}

	for (i=1; i<argc; i++) {
		p = argv[i];
		if (uflg == FALSE) stoupper (p);
		if ((sp = dosopen (p, O_RDWR)) == -1) {
			fprintf (stderr, "%s: Cannot open %s\n", pname, p);
			errcode ++;
			continue;
			}

		/* Must be a MSDOS disc */
		if (!dosfile(sp)) {
			dosclose (sp);
			fprintf (stderr, "%s: %s not a DOS file.\n", pname, p);
			errcode++;
			continue;
			}

		mp = CFD(sp);

		
		mp->dir.attr = mode;
		if (update_dir_entry (mp) == -1) {
			fprintf (stderr, "%s: Cannot write %s\n", pname, p);
			errcode++;
			}
		dosclose (sp);
	}

	mif_quit ();
	exit (errcode);
}

/* -------------------------------------------------------------------- */
/* octal - This routine converts a string to an octal number.  A -1 is  */
/*	   returned if a bad conversion takes place or the number is    */
/*         larger than 255.   						*/
/* -------------------------------------------------------------------- */

int octal (num)
char	*num;
{
	char	c;
	int	i;

	i = 0;
	while (((c= *num++) >= '0') && c <= '7')
		i = (i<<3) + (c-'0');

	/* check bounds on number */
	if ((c != '\0') || (i >= 0xff) || (i<0))
		return (-1);
	return (i);
}

