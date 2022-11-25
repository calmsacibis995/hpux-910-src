/* @(#) $Revision: 56.1 $ */      
/*	DOS utilities program.   All utilities are linked to this program.
	This saves disc space since nearly all utilies use the same core
	functions to access the disc.  This program determines which utility
	was invoked, by looking at the program name.  This results in some
	limitations on what the utilities can be called.
*/
	
#include <fcntl.h>
#include <stdio.h>
#include "dos.h"


#ifdef	DEBUG
extern	boolean	debugon;
#endif	DEBUG

char	*pname;
int	errcode;

/* ----- */
/* main  */
/* ----- */
main (argc, argv)
int	argc;
char	**argv;
{
	int	n;

#ifdef	DEBUG
if ((argc > 1) && (argv[argc-1][0] == '-') && (argv[argc-1][1] == 'x')) {
       debugon = TRUE;
	argv[argc-1] = NULL;
	argc--;
  } else debugon = FALSE;
#endif	DEBUG

	setbuf (stdout, NULL);
	errcode = 0;
	mif_init ();

	/* determine which dos utility was invoked */
	pname = *argv;				/* remember the program name */
	n = strlen(pname)-1;
	if (n < 0)  {
		main_usage ();
	}

	if (pname[n] == 'm')  	dosrm_main (argc, argv);
	  else
	if (pname[n] == 'r')  {
		if (n < 3)  main_usage ();
		if (pname[n-3] == 'm')	dosrm_main (argc, argv);  
				else	dosmkdir_main (argc, argv);  
	  } else
	if (pname[n] == 's')	dosls_main (argc, argv);  
	  else
	if (pname[n] == 'l')	dosls_main (argc, argv);  
	  else
	if (pname[n] == 'f')	dosdf_main (argc, argv);  
	  else
	if (pname[n] == 'p')	doscp_main (argc, argv);  
	  else
	if (pname[n] == 'd')	doschmod_main (argc, argv);
	  else {
		main_usage ();
	}  
}

/*------------*/
/* main_usage */
/*------------*/
main_usage ()
{
fprintf (stderr, "This library uses the program name to determine which");
fprintf (stderr, "utility to invoke.\n");
fprintf (stderr, "The program name %s was not recognized.\n", pname);
fprintf (stderr, "Some valid names are: dosrm, doscp, dosls, dosmkdir,\n");
fprintf (stderr, "dosdf, dosll, dosrmdir, doschmod\n");
exit (1);	
}
