/* @(#) $Revision: 64.1 $ */    
/*	dosrm -irf  (remove a DOS file)
	dosrmdir    (remove a DOS directory)
*/
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include "dos.h"

#define flip(x)	 (x) = !(x)	

char	*malloc (), *strcat (), strncpy (), strcpy (), *strchr (), *strrchr ();
char	*dostail();

#ifdef	DEBUG
extern	boolean	debugon;
#endif	DEBUG

extern char	*pname;
extern int	errcode;

/* ----------- */
/* dosrm_main  */
/* ----------- */
dosrm_main (argc, argv)
int	argc;
char	**argv;
{

	int	fflg;	/* dont complain if file nonexistent or read only   */
	int	iflg;	/* interactively ask to permission to remove a file */
	int	rflg;	/* recursively remove directories and files         */
	int	uflg;	/* dont convert file names to upper case.           */
	int	rmdir_flg; /* flag  set if rmdir was called 		*/
	char	*arg;
	int	n;


	pname = *argv;				/* remember the program name */
	fflg = iflg = rflg = uflg = FALSE;	/* turn off all options  */
	rmdir_flg = FALSE;			/* reset rmdir flag     */
	
	/* decide whether dosrm or dosrmdir was called */
	n = strlen(pname)-3;
	if ((n >= 0) && (strcmp("dir", &pname[n]) == 0)) 
		/* dosrmdir was called */
		rmdir_flg = TRUE;


	if (argc>1 && argv[1][0]=='-') {
		arg = *++argv;
		argc --;
		while (*++arg != '\0')
			switch (*arg) {
				case 'f':	flip(fflg);
						break;
				case 'r':	flip(rflg);
						break;
				case 'i':	flip(iflg);
						break;
				case 'u':	flip(uflg);
						break;
				default: 
				dosrm_usage(rmdir_flg,pname);
					 exit (2);
				}
		}

	/* check for at least one filename */
	if (argc<=1) {
		dosrm_usage(rmdir_flg,pname);
		exit (2);
		}

	if (rmdir_flg == TRUE) { 
		/* dosrmdir was called */
		while (--argc > 0) {
			argv++;
			if (uflg == FALSE) stoupper (*argv);
			dosrmdir (*argv);
		}
		mif_quit();
		exit (errcode?2:0);
		}

	/* dosrm was called */
	while (--argc > 0) {
		if (dotname (*++argv)) {
			fprintf (stderr, "%s: cannot remove . or ..\n", pname);
			continue;
			}
		if (uflg == FALSE) stoupper (*argv);
		dosrm (*argv, fflg, rflg, iflg);

		}
	mif_quit();
	exit (errcode?2:0);
}


/* --------------------------- */
/* dosrm - remove DOS files */
/* --------------------------- */
dosrm (source, fflg, rflg, iflg)
char	*source;
int	fflg;
int	rflg;
int	iflg;
{
	int			i;
	int			sp;
	struct	info		*mp;
	struct	dir_entry	dir;

#ifdef	DEBUG
bugout ("dosrm: source=%s fflg=%x rflg=%x iflg=%x \n", source,fflg,rflg,iflg);
#endif	DEBUG

	if ((sp = dosopen (source, O_RDWR)) == -1) {
		if (errno==ENFILE)
		  fprintf (stderr,
		    "%s: cannot open %s,\n  too many directory levels\n",
		    pname,source);
		else
		  fprintf (stderr, "%s: cannot open %s\n", pname, source);
		errcode++;
		return (0);
		}
	if (!dosfile(sp)) {
		dosclose (sp);
		fprintf (stderr, "%s is not a DOS file\n", source);
		errcode++;
		return (0);
	}
	mp = CFD(sp);

	if ((mp->dir.attr & ATTDIR) == ATTDIR) {	/* delete a directory */

		/* can only delete directories if rflg is set */
		if (!rflg) {
		  dosclose (sp);
		  fprintf (stderr, "%s: %s is a directory\n", pname, source);
		  ++errcode;
		  return (0);
		  }

		/* should we ask permission to remove this directory? */
		if (iflg) {
			printf ("directory %s: ", source);
			if (!yes())  {
				dosclose (sp);
				return (0);
				}
			}

		/* delete all the entries in the directory */
		for (i=0;;i++) {
			char	dosname[13];
			char	name[MAXPATHLEN]; 

			/* If root directory, check for physical end. */
			if (mp->dir.cls == 0 && i >= mp->hdr->dir_entries)
				break;

			/* read entry */
			if (read_dir_entry (mp, i, &dir) < 0) break;

			/* logical end of directory? */ 
			if (dir.name[0] == 0) break;

			/* deleted file? */
			if (dir.name[0] == DIRERASED) continue;

			d_to_u_fname (dosname, dir.name, dir.ext);
			if (dotname(dosname)) continue;
			strcpy (name, source);
			strcat (name, "/");
			strcat (name, dosname);
			dosrm (name, fflg, rflg, iflg);
			}

		dosclose (sp);
		dosrmdir (source);
		return (0);
		}

	/* delete a dos file; not a directory */
	if (iflg) {
		printf ("%s: ", source);
		if (!yes()) return (0);
		}


	mp->dir.name[0] = DIRERASED;
	/* Do not try to deallocate a label entry, it has zero clusters*/
	if (mp->dir.cls != 0) {
		dostruncate (mp, 0);	/* return allocated clusters */
	}
	update_dir_entry (mp);
	dosclose (sp);
	return (0);
}




/* ------------------------------- */
/* dosrmdir -                       */
/* ------------------------------- */
dosrmdir (f)
char	*f;
{
	struct	dir_entry	dir;
	struct	info		*mp;
	int		i;
	int		sp;
	char		*s;

	if ((sp = dosopen (f, O_RDWR)) == -1) {
		fprintf (stderr, "%s: cannot open %s\n", pname, f);
		errcode++;
		return (0);
		}
	if (!dosfile(sp)) {
		dosclose (sp);
		fprintf (stderr, "%s not dos file.  use rmdir to remove it.\n", f);
		errcode++;
		return (0);
		}

	/* make sure we are removing a directory */
	mp = CFD(sp);
	if ((mp->dir.attr & ATTDIR) != ATTDIR) {	
		dosclose (sp);
		fprintf (stderr, "%s: %s not a directory\n", pname, f);
		errcode++;
		return (0);
		}

	/* cant remove . and .. */
	s = dostail (f);
	if (dotname (s)) {
		dosclose (sp);
		fprintf (stderr, "%s: cannot remove . or ..\n", pname);
		errcode++;
		return (0);
		}

	/* make sure that no directory entries remain */
	for (i=0;;i++) {
		char	dosname[14];

		/* If root directory, check for physical end. */
		if (mp->dir.cls == 0 && i >= mp->hdr->dir_entries)
			break;

		/* read entry */
		if (read_dir_entry (mp, i, &dir) < 0) break;

		/* end of directory? */ 
		if (dir.name[0] == 0) break;

		/* deleted file? */
		if (dir.name[0] == DIRERASED) continue;

		/* ignore "." or ".." */
		d_to_u_fname (dosname, dir.name, dir.ext);
		if (dotname (dosname)) continue;
		
		fprintf (stderr, "%s: %s not empty\n", pname, f);
		errcode++; dosclose (sp); return (0);
		}

	/* Do not try to delete root directory. */
	if (mp->dir.cls != 0) {
		mp->dir.name[0] = DIRERASED; /* mark directory "unused" */
		dostruncate (mp, 0);	/* return allocated clusters */
		update_dir_entry (mp);
	}
	dosclose (sp);			/* return data structures */
	return (0);
}



/* ----------------------------------------------------------------- */
/* yes - returns TRUE if the the character gotten from the keyboard  */
/*       is 'Y' or 'y'.  returns FALSE otherwise                     */
/* ----------------------------------------------------------------- */
yes ()
{
	int	i;
	int	b;

	i = b = getchar ();
	while (b != '\n' && b > 0)
		b = getchar ();
	return (i=='y' || i=='Y');
}




/* ------------------------------------------------------ */
/* dotname -  returns TRUE if the string s is "." or ".." */
/*            returns FALSE otherwise                     */
/* ------------------------------------------------------ */
boolean dotname (s)
char	*s;
{
	if (s[0] == '.')
		if (s[1] == '.')
			if (s[2] == '\0')
				return (TRUE);
			else 
				return (FALSE);
		else if (s[1] == '\0')
			return (TRUE);
	return (FALSE);
}

/* ------------------------------------------------------ */
/* dosrm_usage -  prints out the proper usage	          */
/*          according to the rmdir_flg                    */
/* ------------------------------------------------------ */
int dosrm_usage(flag,name)
int  flag;
char *name;
{
	/* if flag is set print out rmdir usage,otherwise rm usage */
	if(flag == FALSE)
		fprintf (stderr, "usage: %s [-fiur] file ...\n",name);
	else
		fprintf (stderr, "usage: %s [-u] directory ...\n",name);

	return (0);
}
