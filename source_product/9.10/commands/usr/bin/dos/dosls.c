/* @(#) $Revision: 64.1 $ */    
/*	List DOS files.
		Issues:
*/
#include <fcntl.h>
#include <stdio.h>
#include "dos.h"

#define flip(x)	 (x) = !(x)	

char	*malloc (), *strcat (), strncpy (), strcpy (), *strchr ();

#ifdef DEBUG
extern	boolean	debugon;
#endif DEBUG

extern char	*pname;
int	ls_errcnt	= 0;
boolean	long_listing	= FALSE;
boolean directory_only	= FALSE;
boolean	list_all	= FALSE; /* list hidden and system files */
boolean list_dots	= FALSE;
boolean uflg		= FALSE; /* convert file names to upper case */
boolean flag_type	= FALSE; /* give an indication of the type of file */
boolean raw		= FALSE; /* print files in the order that they are */
				 /* found in the directory.                */

char	*months[] = { "*****", "Jan  ", "Feb  ", "March", "April", "May  ",
		      "June ", "July ", "Aug  ", "Sept ", "Oct  ", "Nov  ",
		      "Dec  "};


/* ----------- */
/* dosls_main  */
/* ----------- */
dosls_main (argc, argv)
int	argc;
char	**argv;
{

	/* if the name of program ends in 'l' then do a long listing */
	pname = *argv++;
	if (pname[strlen(pname)-1] == 'l')
		long_listing = TRUE;

	if (getuid () == 0)		/* superuser => list everything */
		list_all = TRUE;
	
	/* process program arguments */
	while (*argv!=NULL && **argv=='-') {
		register char *s;

		for (s = *argv++ + 1; *s; s++) {
			switch (*s) {
			    case 'A' : flip (list_all);			break;
			    case 'a' : list_all = list_dots = TRUE;	break;
			    case 'd' : flip (directory_only);		break;
			    case 'l' : flip (long_listing);		break;
			    case 'r' : flip (raw);			break;
			    case 'u' : flip (uflg);			break;
			    default : goto usage;
			}
		}
	}

	if (*argv==NULL) { 		/* => no files to list */
usage:
	  fprintf (stderr, "usage: %s -Aaudl device:file ... \n", pname );
	  exit(1);
	}
	
	/* process the files/directories in the argument list.  Flush the */
	/* output queue after each.					  */
	for ( ; *argv!=NULL; argv++) {
		if (uflg == FALSE) stoupper (*argv);
		dosls (*argv);
		flushit ();
	}
	mif_quit ();
	exit (ls_errcnt? 1 : 0);
}


/* ---------------------------- */
/* dosls - list a dos directory */
/* ---------------------------- */
dosls (path)
char	*path;
{
	int			i;
	int			dp;
	struct	info		*mp;
	struct	dir_entry	dir;	

#ifdef	DEBUG
bugout ("dosls: path=%s \n", path);
#endif	DEBUG

	dp = dosopen (path, O_RDONLY);
	if (dp ==-1) {
	  fprintf(stderr,"%s: cant open %s for listing.\n",pname,path);
	  ls_errcnt++;
	  return (-1);
	}
	if (!dosfile(dp)) {
	  fprintf(stderr,"%s: %s does not appear to be a  DOS directory.\n", 
		pname, path);
	  fprintf(stderr,"%s: DOS names need a colon in the file name path.\n",
		 pname);
	  dosclose (dp);
	  ls_errcnt++;
	  return (-1);
        }
	
	mp = CFD(dp);

	if (((mp->dir.attr & ATTDIR) != ATTDIR) || directory_only) {
	  dosclose (dp);
	  enqueue (path);
	  return (0);
	}

	for (i=0;;i++) {
		char	buf[MAXPATHLEN]; 
		char	new_name[13];


		/* If root directory, check for physical end of directory.
		   Entries are numbered from 0 thru (dir_entries-1). */
		if (mp->dir.cls == 0 && i >= mp->hdr->dir_entries)
			break;
		
		/* read entry */
		if (read_dir_entry (mp, i, &dir) < 0) break;

/*------------------------------------------------------------------*/
/* Check to see if file is really a label before going on to others */
/* and then print it out if it is a label, long_listing is set      */
/* and it is not in raw mode, ignore label file otherwise	    */
/*								    */
/*------------------------------------------------------------------*/
	if(((dir.attr & ATTVOLLBL) == ATTVOLLBL) && (!raw)) {
	  if(long_listing) {
	    printf("\nThe DOS Volume Label is  %s%s\n",dir.name,dir.ext); 
	    printf(" \n"); 
	    continue;
	  }
	  else  continue;
	} 
		/* logical end of directory? */ 
		if (dir.name[0] == DIRUNUSED) break;

		/* should directory entry be dumped */
		if (raw) {
			print_raw (dir);
			continue;
		}

		/* deleted file? */
		if (dir.name[0] == DIRERASED) continue;


		d_to_u_fname (new_name, dir.name, dir.ext);

		/* dont list . or .. unless requested to do so */
		if (   (  (strcmp (new_name, ".") == 0)
			||(strcmp (new_name, "..") == 0))
		    && (!list_dots))
			continue;

		/* dont list system or hidden files unless requested to do so */
		if ((((dir.attr & ATTHIDDEN) == ATTHIDDEN)
		    || ((dir.attr & ATTSYS) == ATTSYS))  
		    && (!list_all))
			continue;


		strcpy (buf, path);
		if (buf[strlen(buf)-1] != '/')
			strcat (buf, "/");
		strcat (buf, new_name);
		enqueue (buf);
	}
	flushit ();
	dosclose (dp);
	return (0);
}


/* ------------------ */
/* list_file - print */
/* ----------------- */
list_file (path)
char	*path;
{
	int	month;
	int	ret;
	struct dir_entry	dir;
	ret = dosstat (path, &dir);
	if (ret == -1) {
		fprintf (stderr, " %s: can't stat %s\n", pname, path);
		ls_errcnt++;
		return (-1);
	}

	if (!long_listing) {
		printf ("%s", path);
		print_flag_char (dir);
		puts ("");
		return (0);
	}

	/* attributes */
	printf ("%3o", dir.attr);
	printf (" %7d", dir.size);
	if (dir.month < 1 || dir.month > 12) 
		month = 0;
	else month = dir.month;
	printf (" %5.5s %2.2d %4.4d  %2.2d:%2.2d  ", months[month], dir.day,
		dir.year+1980, dir.hour, dir.minute );
	printf ("%s", path);
	print_flag_char (dir);
	puts ("");
	return (0);	
}




/* ------------------*/
/* print_raw - */
/* ------------------*/
print_raw (dir)
struct dir_entry	dir;
{

	printf ("%3o", dir.attr);
	printf (" %4x", dir.cls);
	printf (" %8x", dir.size);
	printf (" %2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d", dir.month, dir.day,
		dir.year, dir.hour, dir.minute, dir.second );
	printf (" %s", dir.name);
	printf (" %s", dir.ext);
	print_flag_char (dir);
	puts ("");
	
}


/* ------------------*/
/* print_flag_char - */
/* ------------------*/
print_flag_char (dir)
struct dir_entry	dir;
{
	if ((dir.attr & ATTDIR) == ATTDIR)
		printf ("/");
}


#define BUFSIZE	512
char *ptrbuf[BUFSIZE];
int	ptrcount = 0;

/* ----------*/
/* enqueue - */
/* ----------*/
enqueue (path)
char	*path;
{
	static boolean	message_printed = FALSE;
	char	*p;

	if (ptrcount>=BUFSIZE) {
		if (!message_printed) {
			fprintf (stderr, "%s: too many files to sort\n", pname);
			message_printed = TRUE;
			ls_errcnt++;
		}
		flushit ();
	}
	
	p = malloc ((unsigned) (strlen (path) +1));
	if (p == NULL) {
		fprintf ("%s: can't allocate memory\n", pname);
		list_file (path);
		ls_errcnt++;
	}
	
	strcpy (p, path);
	ptrbuf[ptrcount++] = p;
}


/* ----------*/
/* compare - */
/* ----------*/
compare (a, b)
char	**a;
char	**b;
{
	return (strcmp (*a, *b));
}



/* ------------------------ */
/* flushit - sort and print */
/* ------------------------ */
flushit ()
{
	char	**p;

	qsort ((char *) ptrbuf, ptrcount, sizeof(ptrbuf[0]), compare);
	for (p=ptrbuf; ptrcount>0; ptrcount--, p++) {
		list_file (*p);
		free (*p);
	}
}
