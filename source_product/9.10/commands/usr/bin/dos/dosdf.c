/* @(#) $Revision: 63.2 $ */    
/*	Print the number of free disc blocks on a DOS volume
*/
#include <stdio.h>
#include <fcntl.h>
#include "dos.h"

#define flip(x)	 (x) = !(x)	

char	*malloc (), *strcat (), strncpy (), strcpy (), *strchr (), *strrchr ();
struct	header	*open_device ();

#ifdef DEBUG
extern	boolean	debugon;
#endif DEBUG
extern	char	*pname;


/* ----------- */
/* dosdf_main  */
/* ----------- */
dosdf_main (argc, argv)
int	argc;
char	**argv;
{

	int	cls;
	int	n;
	int	unallocated;
	char	*p;
	struct	header	*hdr;

	pname = *argv++;			/* remember program name */
	argc--;

	/* Make sure we have a single argument */
	if (argc != 1) {
		fprintf (stderr, "Usage: %s device\n", pname);
		exit(2);
	}

	for (p = *argv; (*p!='\0') && (*p!=':'); p++);	/* check proper device name */
	*p = '\0';

	if ((hdr = open_device (*argv, O_RDONLY)) == NULL) {
		fprintf (stderr, "%s: Cant access %s\n", pname, *argv);
		exit (2);
		}

	unallocated = 0;

	/* FAT numbers clusters 2 thru max_cls, so max_cls is one larger
	   than number of physical clusters. */
	for (n=2; n<=hdr->max_cls; n++) {
	  if ((cls = get_fat_entry (hdr, n)) == -1)
		fprintf (stderr, "%s: Trouble reading FAT. quitting.\n", pname);
	  if (cls==0) unallocated++;
	  }

	fprintf (stdout, "%s:\n", pname);
	fprintf (stdout, "\tdisc capacity = %d bytes\n", 
		 hdr->total_sec*hdr->byt_p_sector);
	fprintf (stdout, "\tcluster size = %d bytes\n", hdr->byt_p_cls);
	fprintf (stdout, "\t%d clusters allocated. \n\t%d clusters free.\n", 
		 hdr->max_cls-unallocated-1, unallocated);
	close_device (hdr);
	mif_quit ();
	exit (0);
}
