/*	@(#) $Revision: 70.2 $	*/
static char *RCS_ID="@(#)$Revision: 70.2 $ $Date: 91/11/07 10:49:08 $";
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else /* NLS */
#define NL_SETN 1	
#include <nl_types.h>
#endif /* NLS */

/* HoneyDanBer uuls */
/*
 * List files in a uucp spool directory grouped by transaction.
 * See the manual entry for details.
 *
 * Compile with -lndir (uses directory(3)).
 * Compile with -DUCB if strchr(3s) or getopt(3) are not supported.
 * Compile with -DBSEARCH if bsearch(3c) is not supported.
 * Includes HP-proprietary (newly-written) getopt() and bsearch().
 *
 * Enhancement:  ListFiles(), while listing Others, could be sped up by
 * skipping all control and subfiles, but that would mean remembering three
 * different sections of the heap so as to scan the other four parts.  Of
 * course, then you could stop marking any files as used except subfiles, and
 * not have to check in ListFile() if a filename is already used.
 *
 * Bug:  If asked to do more than one directory and any but the first
 * is a relative (not absolute) pathname, chdir() will fail.  The
 * alternatives are all horrible:
 *
 *	1:  Fork child process for each directory so it can chdir() safely.
 *	2:  Stick the full pathname in front of each file fopen'd (slow).
 *	3:  Remember pwd and return to it for each new directory (maybe...).
 */


#ifdef UCB
#define	strchr index
#endif

#include <stdio.h>
#include <ndir.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BSEARCH            /* bsearch(3C) does not points to
                              the first matched entry when there 
                              are more than one match. So use the
                              local one 
                            */ 
#ifndef BSEARCH
#include <search.h>
#endif


/************************************************************************
 * PRINTING FORMAT CONTROL:
 *
 * Newlines are printed on an as-needed basis, prior to printing data,
 * except that some routines do print a newline after data, and then
 * update nextcol.  Only the Print routines modify nextcol.
 *
 * Sizes are carefully chosen based on what fits on a line and how data
 * lines up.  Some are also used for data declarations and truncating
 * long strings.  See actual uses of the numbers for details.
 */

int	nextcol = 1;				/* next column to print	*/

#define	NEWTRANS  0				/* modes for PrintFile() */
#define	SUBFILE	  1
#define	ORDINARY  2

#define	MISSING   ('*')				/* precedes missing file */
#define	MYDIRSIZ  MAXNAMLEN			/* filename in dir entry */
#define	NAMESIZE  (MYDIRSIZ + 1)		/* filename and null	 */
#define	FILESIZE  (MYDIRSIZ + 2)		/* filename, null, mark	 */
#define	FILESIZEL ((long) FILESIZE)
#define MINSIZE   14                            /* old filename length	 */
/*#define MINSIZE   (DIRSIZ + NAMESIZE - MYDIRSIZ) old filename length	 */

#define	INDLINE	  2				/* indent filename lines */
#define	INDFILE	  2				/* before each filename	 */
#define	INDSUB	  (INDLINE + INDFILE + MINSIZE) /* before subfile line  */

#define	NODESIZE  7				/* longest nodename	*/
#define	USERSIZE  8				/* longest username	*/
#define	NODEUSER  (NODESIZE + 1 + USERSIZE)	/* node!user		*/

#define	PRINTLIM  80				/* stop before this col	*/
#define	CMDSIZE   /* maximum size of D*X* command line */	\
	    (PRINTLIM - INDLINE - INDFILE - MINSIZE - INDFILE - NODEUSER - 2)


/************************************************************************
 * MISC GLOBALS:
 */

#define	proc				/* null; easy to grep for procs */
#define	chNull	 ('\0')
#define	chMark	 ('\1')			/* to mark used-up filenames */
#define	sbNull	 ((char *) NULL)
#define	fileNull ((FILE *) NULL)

char	*myname;				/* how command invoked	*/
int	kflag   = 0;				/* -k (kbytes) option	*/
int	mflag   = 0;				/* -m (meanings) option	*/
int	sflag   = 0;				/* -s (size) option	*/
int	ksflag  = 0;				/* k or s: need stat()s	*/
long	filenames;				/* total files in heap	*/

char	*DEFAULTARGS[] = { "/usr/spool/uucp", sbNull };
#ifdef NLS
nl_catd nlmsg_fd;
#endif


/************************************************************************
 * MACROS:
 */

#define	PRINTERR(part1,part2) \
		fprintf (stderr, "%s: %s %s\n", myname, part1, part2);

/*
 * Check if at separator:
 * Note that newline and chNull are also seen as separators for NOTSEP().
 */

char	*strchr();
#define	SEP(cp)    ((*(cp) == ' ') || (*(cp) == '\t'))	/* at separator	    */
#define	NOTSEP(cp) (strchr (" \t\n", *(cp)) == sbNull)	/* not at separator */


/************************************************************************
 * M A I N
 */

proc main (argc, argv)
	int	argc;
	char	**argv;
{
	extern	int	optind;			/* for getopt()		  */
	int	optchar;			/* for getopt()		  */
	char	*heapstart, *heaplim;		/* heap start, end + 1	  */
        char    *dstart,*dlim;
        int hdb;
#ifdef NLS
	nlmsg_fd = catopen("uucp",0);
#endif

	myname = *argv;
        hdb = 0;
        

/*
 * CHECK ARGUMENTS:
 */

	while ((optchar = getopt (argc, argv, "kms")) != EOF)
	    if	    (optchar == 'k')	kflag = 1;
	    else if (optchar == 'm')	mflag = 1;
	    else if (optchar == 's')	sflag = 1;
	    else			Usage ();

	if (kflag && sflag)			/* mutually exclusive */
	    Usage ();

	ksflag = (kflag || sflag);		/* file stats are needed */
	argc  -= optind;
	argv  += optind;

#ifndef HDBuucp /*HDB UUCP */      
	if (argc < 1)				/* use default arg list */
	    argv = DEFAULTARGS;
#else 
        /* if HoneyDanBer UUCP, prepare the contents of /usr/spool/uucp
           into a heap */

        if (argc < 1){
                     hdb++;  /* use defaults */
                     PrepDir("/usr/spool/uucp", & dstart,& dlim,1);
         }
#endif
 


/*
 * DO EACH DIRECTORY:
 *
 * Read contents of each directory into heap, sort list of files, list files,
 * and return heap space:
 */

#ifdef HDBuucp /* HDB */
   /* if HoneyDanBer UUCP, do each subdirectory in /usr/spool/uucp
      separately */
   /* dstart points to a heap of filenames in /usr/spool/uucp */

   if (hdb){  /* default -- /usr/spool/uucp/* */
	for ( ; *dstart != sbNull; dstart+=FILESIZE)
	{
	    heapstart = sbNull;
	    if (PrepDir (dstart, & heapstart, & heaplim,0)) /* files read OK */
		ListFiles (dstart, heapstart, heaplim);

	    if (heapstart != sbNull)
		free (heapstart);
	}
   }else{
	for ( ; *argv != sbNull; argv++)
	{
	    heapstart = sbNull;
	    if (PrepDir (*argv, & heapstart, & heaplim,0)) /* files read OK */
		ListFiles (*argv, heapstart, heaplim);

	    if (heapstart != sbNull)
		free (heapstart);
	}
   }
#else
	for ( ; *argv != sbNull; argv++)
	{
	    heapstart = sbNull;
	    if (PrepDir (*argv, & heapstart, & heaplim,0)) /* files read OK */
		ListFiles (*argv, heapstart, heaplim);

	    if (heapstart != sbNull)
		free (heapstart);
	}
#endif

#ifdef NLS
	catclose(nlmsg_fd);
#endif

} /* main */


/************************************************************************
 * U S A G E
 *
 * Tell correct usage.
 */

proc Usage ()
{
	fprintf (stderr, (catgets(nlmsg_fd,NL_SETN,861, "usage:  %s    [-m] [directories...]\n")), myname);
	fprintf (stderr, (catgets(nlmsg_fd,NL_SETN,862, "        %s -s [-m] [directories...]\n")), myname);
	fprintf (stderr, (catgets(nlmsg_fd,NL_SETN,863, "        %s -k [-m] [directories...]\n")), myname);
	exit (1);
} /* Usage */


/************************************************************************
 * P R E P   D I R
 *
 * Prepare a directory:  read filenames from directory into heap space
 * and sort them.  Return zero for failure or one for success, in which
 * case heap space is allocated, and heapstart, heaplim, and filenames
 * are set.  Regardless of return, caller must free heap space, if any.
 *
 * Pre-allocates one contiguous chunk of memory the size of the directory
 * plus 1Kb to allow for it growing as it's read (or the most it can get),
 * to hold filenames for sorting and searching.  Assumes this is big
 * enough to hold all the actual filenames and usage marks, at FILESIZE
 * bytes each, but checks for overrun while reading in, just to be sure.
 *
 * Doesn't use sbrk() directly (as it did at one time) because some
 * library routines call malloc(), which leads to trouble.  Doesn't read
 * the directory an extra time up front to count actual files because that
 * would slow things down.
 */

proc PrepDir (dirname, heapstartp, heaplimp, opt)
	char	*dirname;			/* name of directory	*/
	char	**heapstartp;			/* heap start (to set)	*/
	char	**heaplimp;			/* end + 1 (to set)	*/
        int     opt;                            /* 1 for HDB /usr/spool/uucp */
{
register char	*heapstart;			/* fast local copies	*/
register char	*heaplim;
	 DIR	*dirp;				/* currently open dir	*/
register struct	direct *dp;			/* one file entry	*/
register char	*filename;			/* fast copy of it	*/
	 struct	stat statbuf;			/* returned from stat()	*/
	 struct	stat tb;			/* returned from stat()	*/
	 unsigned heapavail;			/* bytes malloc'd	*/

        char    tmp[1024];
	char	*malloc();
	int	strcmp();

/*
 * GET SIZE OF AND OPEN DIRECTORY for reading:
 */

	if (stat (dirname, & statbuf) < 0)
	{
	    PRINTERR ((catgets(nlmsg_fd,NL_SETN,864, "can't stat directory")), dirname);
	    return (0);
	}

	if ((dirp = opendir (dirname)) == (DIR *) NULL)
	{
	    PRINTERR ((catgets(nlmsg_fd,NL_SETN,865, "can't opendir directory")), dirname);
	    return (0);
	}
	/* now remember to close dirp before returning */

/*
 * GET MEMORY TO HOLD FILENAMES:
 */

	/*heapavail = statbuf.st_size + 1024;*/	/* arbitrary extra amount */
	heapavail = statbuf.st_size * FILESIZE / DIRSIZ;
        /*if (opt){
                heapavail = heapavail + strlen(dirname)*FILESIZE/DIRSIZ;
	    
	}*/

	while ((heapavail > 0)
	&&     ((heapstart = malloc (heapavail)) == sbNull))
	{
	     heapavail -= 1024;			/* try for a little less */
	}

	*heapstartp = heaplim = heapstart;

/*
 * READ EACH DIRECTORY ENTRY AND SAVE FILENAME IF VALID:
 * Be sure there is enough room in heap for each added filename.
 */

	while ((dp = readdir (dirp)) != (struct direct *) NULL)
	{
	    filename = dp->d_name;

	    if ((dp->d_ino)				/* valid entry	*/
	    &&	(filename[0] != chNull)			/* has filename	*/
	    &&	(strncmp (filename, ".",  2))		/* valid file	*/
	    &&	(strncmp (filename, "..", 3)))
	    {
		if (heapavail <= FILESIZE)		/* out of room */
		{
		    PRINTERR ((catgets(nlmsg_fd,NL_SETN,866, "directory too large")), "");
		    closedir (dirp);
		    return (0);
		}
                /* Only put subdirectory name into the heap
                   if we are preparing /usr/spool/uucp */
                if (opt) {       
                          strcpy(tmp,dirname);
                          strcat(tmp,"/");
                          strcat(tmp,filename);
                          if (!stat(tmp,&tb) &&       
                              ((tb.st_mode & S_IFMT) == S_IFDIR)){
                          	filename=tmp;
                          }else continue;
                }
		strncpy (heaplim, filename, MYDIRSIZ);	/* save the name  */
		heaplim [MYDIRSIZ] = chNull;		/* add terminator */
		heaplim [NAMESIZE] = chNull;		/* add marker	  */

		heaplim   += FILESIZE;
		heapavail -= FILESIZE;
	    }
	}
        if (opt){ 	/* finally, put the directory name itself */
                        /* if this is a true HDB system, it will just
                           give all the subdirectories */
		strncpy (heaplim, dirname, MYDIRSIZ);	
		heaplim [MYDIRSIZ] =  0;	
		heaplim [NAMESIZE] =  0;

		heaplim   += FILESIZE;
		heapavail -= FILESIZE;
       }
	closedir (dirp);

/*
 * SORT FILENAMES IN HEAP:
 */

	if (heaplim <= heapstart)			/* no actual files */
	    return (0);

	filenames = (heaplim - heapstart) / FILESIZE;
	qsort (heapstart, (int) filenames, FILESIZE, strcmp);

	*heaplimp = heaplim;
	return (1);

} /* PrepDir */


/************************************************************************
 * L I S T   F I L E S
 *
 * List files based on filenames sorted in heapspace, in three phases:
 *
 * 1: List control ("C.*", "X.*") files and the subfiles they reference;
 * 2: List orphans (remaining "D.*" files);
 * 3: List other files (any not already referenced).
 *
 * During phases 1 and 2, filenames are marked as used, by setting an
 * extra char (after null terminator) to chMark.  The names themselves
 * are kept intact so bsearch() works.
 */

proc ListFiles (dirname, heapstart, heaplim)
	char	*dirname;			/* directory being done	*/
	char	*heapstart, *heaplim;		/* heap start, end + 1	*/
{
register char	*filename;			/* name of current file	*/
	 long	totsize;			/* total trans bytes	*/
register int	orphans = 0;			/* any orphans seen?	*/
register int	others  = 0;			/* any others  seen?	*/

	long	ListTrans();
	char	*bsearch();
	int	Str2Cmp();
        char    Dirname[1024];

/*
 * GO TO DIRECTORY TO BE LISTED:
 */
 
	if (chdir (dirname) < 0)
	{
	    PRINTERR ((catgets(nlmsg_fd,NL_SETN,867, "can't chdir to")), dirname);
	    return;
	}
#ifdef HDBuucp /* HDB */
        sprintf(Dirname,"%s :",dirname);
        PrintLine(Dirname);
#endif

/*
 * LIST TRANSACTIONS, first "C" files, then "X" files:
 *
 * ListTrans() does the checking for no filename found by bsearch().
 */

	filename = bsearch ("C.", heapstart, filenames, FILESIZEL, Str2Cmp);
	totsize  = ListTrans (heapstart, filename, heaplim);

	filename = bsearch ("X.", heapstart, filenames, FILESIZEL, Str2Cmp);
	totsize += ListTrans (heapstart, filename, heaplim);

/*
 * PRINT TRANSACTION TOTALS IF NEEDED:
 *
 * Note: if there are no transactions, totsize == 0 so we don't print
 * anything, which is good because there is no section header in this case.
 */

	if (ksflag && totsize)
	{
	    PrintFile (NEWTRANS, 0, (catgets(nlmsg_fd,NL_SETN,869, "total")));	/* not missing file	 */
	    PrintSize (totsize);		/* finishes off the line */
	}

/*
 * LIST ORPHANED SUBFILES:
 *
 * Only search the part of the (sorted) filename list that applies.
 */

	filename = bsearch ("D.", heapstart, filenames, FILESIZEL, Str2Cmp);

	if (filename != sbNull)				/* one found */
	{
	    for ( ;
		 (filename < heaplim) && (strncmp (filename, "D.", 2) == 0);
		 filename += FILESIZE)
	    {
		if (filename [NAMESIZE] == chNull)	/* name not used  */
		{					/* it's an orphan */
		    if (! orphans)			/* first one seen */
		    {
			PrintLine ((catgets(nlmsg_fd,NL_SETN,870, "Orphans:")));		/* print header */
			orphans = 1;
		    }

		    PrintFile (ORDINARY, 0, filename);	/* not missing file  */
		    filename [NAMESIZE] = chMark;	/* mark name as used */
		}
	    }
	}

/*
 * LIST OTHER FILES:
 */

	for (filename = heapstart; filename < heaplim; filename += FILESIZE)
	{
	    if (filename [NAMESIZE] == chNull)		/* name not used yet */
	    {
		if (! others)				/* first one seen */
		{
		    PrintLine ((catgets(nlmsg_fd,NL_SETN,871, "Others:")));		/* print header	*/
		    others  = 1;
		}

		PrintFile (ORDINARY, 0, filename);	/* not missing file */
	    }
	}

	printf ((catgets(nlmsg_fd,NL_SETN,872, "\n")));					/* finish last line */

} /* ListFiles */


/************************************************************************
 * S T R   2   C M P
 *
 * Just a way to get strncmp() called from bsearch(), in order to find
 * the first filename beginning with (not wholly matching) a pattern
 * of size two chars.
 */

int Str2Cmp (string1, string2)
	char	*string1, *string2;	/* strings to compare */
{
	return (strncmp (string1, string2, 2));
} /* Str2Cmp */


/************************************************************************
 * L I S T   T R A N S
 *
 * List transactions of one kind based on filenames in heap space, and
 * return the total transaction bytes for this kind (if ksflag).  The
 * starting filename is set to the first "C" or "X" file to list, or
 * sbNull if there are none.  Lists each one and the subfiles it
 * references, with optional stats and prints of transaction sizes.
 */

proc long ListTrans (heapstart, filename, heaplim)
	char	*heapstart;			/* heap start	     */
register char	*filename;			/* starting filename */
register char	*heaplim;			/* heap end + 1	     */
{
static	int	title = 0;			/* any trans seen?	*/
						/* only init'd once!	*/
	char	kind [2];			/* current control kind	*/
	FILE	*filep;				/* open control file	*/
	int	typeC;				/* type C, not X?	*/

	long	transize;			/* bytes in transaction	*/
	long	totsize = 0;			/* total bytes		*/
	long	ListSubs();

	if (filename == sbNull)			/* none of this type */
	    return (0L);

/*
 * SCAN CONTROL FILES; quit when hit a different kind of file:
 */

	strncpy (kind, filename, 2);		/* save kind in progress */
	typeC = (kind [0] == 'C');

	for ( ;
	     (filename < heaplim) && (strncmp (filename, kind, 2) == 0);
	     filename += FILESIZE)
	{
	    filename [NAMESIZE] = chMark;	/* mark as used */

	    if (! title)			/* first one seen */
	    {
		PrintLine ((catgets(nlmsg_fd,NL_SETN,873, "Transactions:")));	/* print header */
		title = 1;
	    }

/*
 * OPEN CONTROL FILE, THEN PRINT NAME (as missing if open fails):
 */

	    filep = fopen (filename, "r");

	    PrintFile (NEWTRANS, (filep == fileNull), filename);

	    if (filep == fileNull)		/* open failed */
	    {
		if (ksflag)
		    PrintSize (0L);		/* show zero, finish line */
		continue;
	    }
	    /* now remember to close filep */

/*
 * LIST DATA FOR ONE CONTROL FILE:
 */

	    if (mflag && (! typeC))		/* need meaning of "X" file */
		PrintMeaningDX (filename);

	    transize = ListSubs (filep, typeC, heapstart);
	    fclose (filep);

	    if (ksflag)
	    {
		PrintSize (transize);		/* finishes off the line */
		totsize += transize;
	    }

	} /* for */

	return (totsize);

} /* ListTrans */


/************************************************************************
 * L I S T   S U B S
 *
 * List subfiles (or meanings) of one transaction, of type "C" or "X", and
 * return the total transaction size.
 */

proc long ListSubs (filep, typeC, heapstart)
register FILE	*filep;				/* open control file	*/
	int	typeC;				/* type "C" or "X"?	*/
	char	*heapstart;			/* heap start		*/
{
register char	*subname;			/* subfile within trans	*/
	char	*subfile;			/* subfile in heap	*/
	struct	stat	statbuf;		/* returned from stat()	*/
	int	receive;			/* subfile to receive?	*/
	int	samedir;			/* subfile in same dir?	*/
	int	missing;			/* subfile missing?	*/
	int	meaning  = 0;			/* printed a meaning?	*/
	long	transize = 0L;			/* transaction size	*/

	char	*GetSubName();
	char	*bsearch();

/*
 * SCAN SUBFILES:
 *
 * To save time, no username is requested from GetSubName().
 */

	while ((subname = GetSubName (filep, typeC, & receive, & samedir,
				      (char **) sbNull)) != sbNull)
	{
	    if (receive)			/* not of interest now */
		continue;

	    missing = 0;

/*
 * CHECK FOR LOCAL SUBFILES IN THE SORTED LIST:
 *
 * Assumes filenames in the list are unique in all MYDIRSIZ characters.
 * (Should be OK given the uucp filenaming scheme.)
 */

	    if (samedir)				/* normal subfile */
	    {
		subfile = bsearch (subname, heapstart, filenames,
				   FILESIZEL, strcmp);

		if ((subfile == sbNull) || (subfile [NAMESIZE] == chMark))
		    missing = 1;		/* missing or already used */
		else
		    subfile [NAMESIZE] = chMark;	/* mark as used */
	    }

/*
 * GET SIZE OF ONE SUBFILE, if needed (including not-samedir files):
 */

	    if (ksflag && (! missing)
	    &&  ((missing = stat (subname, & statbuf)) == 0))
	    {
		transize += statbuf.st_size;	/* OK to add size */
	    }

/*
 * PRINT DATA FOR SUBFILE, if needed:
 *
 * Don't print not-samedir subfiles.
 * Don't print subfile meaning if "X" control file (was already done).
 */

	    if (samedir)
	    {
		if (! mflag)				/* normal case */
		    PrintFile (SUBFILE, missing, subname);
		else					/* show meanings */
		{
		    if (typeC)
			meaning |= PrintMeaningDX (subname);
		}
	    }
	} /* while */

/*
 * PRINT MEANING OF "C" FILE if needed but not yet done:
 */

	if (mflag && typeC && (! meaning))
	{
	    rewind (filep);
	    PrintMeaningC (filep);
	}

	return (transize);

} /* ListSubs */


/************************************************************************
 * G E T   S U B   N A M E
 *
 * Get next subfilename from a control file.  If a subfile is found,
 * return *subname, samedir and receive flags valid, and **user if needed
 * (if caller passes a valid pointer, not sbNull).  Returns sbNull at the
 * end of the control file.  subname and user are only valid until the
 * next call.
 *
 * subname is never more than MYDIRSIZ non-null chars unless it's a
 * not-samedir subfile (see below).  user is never more than USERSIZE.
 *
 * Subfilenames come from control file lines, at most one per line.
 * They are found as blank-separated fields in lines in "C" or "X" files:
 *
 *	C:  R <remotefrom> <localto> <user> -<options>
 *	C:  S <localfrom> <remoteto> <user> -<options> <subfile> <mode>
 *	X:  F <subfile>
 *
 * In the "R" (receive) case, <remotefrom> is used as the subfile name,
 * receive flag is returned true, and samedir flag, false.
 *
 * The receive flag is false for all other cases.
 *
 * In the "S" (send) case, if <subfile> is "D.0", <localfrom> is a file
 * not in the spool directory (a not-samedir subfile), derived from a uucp
 * call without the -C (copy) option.  It is used as the subfile name and
 * the samedir flag is returned false.
 *
 * In the "F" (exec file) case, the samedir flag is always true.
 *
 * Uucp -C and uux both set <subfile> to a true (spooled) subfile.
 */

proc char * GetSubName (filep, typeC, receivep, samedirp, userp)
	FILE	*filep;				/* open control file	*/
	int	typeC;				/* type "C" or "X"?	*/
	int	*receivep;			/* is receive?		*/
	int	*samedirp;			/* is same directory?	*/
	char	**userp;			/* to return username	*/
{
static	char	line [BUFSIZ];			/* read from cntrl file	*/
register char	*subname;			/* found in line	*/
register char	*cp;				/* for scanning line	*/
register char	*cpend;				/* end of filename	*/
	int	receive;			/* is a receive file?	*/
	int	notsamedir;			/* not in same dir?	*/
	char	*typestr = typeC ? "S " : "F ";	/* line type needed	*/

/*
 * READ LINES AND CHECK FIRST LETTERS:
 */

	while (fgets (line, BUFSIZ, filep) != sbNull)
	{
	    receive = typeC && (strncmp (line, "R ", 2) == 0);

	    if ((! receive) && (strncmp (line, typestr, 2)))
		continue;			/* line not of interest */

	    subname = line;

/*
 * FOR "C" FILE, SET UP SUBNAME AND USERNAME:
 *
 * Find char before start of options (" -").  Skip line if none found.
 * Find username if needed and mark end.  Then, if a receive line,
 * prepare to find <remotefrom>.
 */

	    if (typeC)
	    {
		while ((*subname != chNull) && strncmp (subname, " -", 2))
		    subname++;			/* now at " -" */

		if (*subname == chNull)		/* missing options */
		    continue;

		if (userp != (char **) sbNull)	/* caller wants username */
		{
		    for (cp = subname - 1; (cp > line) && NOTSEP (cp); cp--)
			;
		    *userp = ++cp;		/* first char of username */

		    if ((subname - cp) > USERSIZE)
			cp [USERSIZE] = chNull;		/* truncate name */
		    else
			*subname = chNull;		/* mark end */
		}

		if (receive)			/* treat as not-same-dir */
		    subname = "- D.0";
		else
		    subname++;
	    }
	    /* now (*subname == '-') */

/*
 * SKIP TO NEXT SEPARATOR AND PASS IT:
 */

	    while (NOTSEP (subname))
		subname++;

	    while (SEP (subname))
		subname++;			/* now at subname */

/*
 * CHECK FOR NON-LOCAL SUBFILE:
 *
 * If so, set subname to start of <localfrom> (same field as <remotefrom>).
 * Char after name might be newline or chNull, so use (! NOTSEP()).
 */

	    notsamedir = typeC && (strncmp (subname, "D.0", 3) == 0)
			 && (! NOTSEP (subname + 3));

	    if (notsamedir)
		for (subname = line + 2; SEP (subname); subname++)
		    ;

/*
 * CHECK SUBNAME AND SKIP IT:
 *
 * Length of name is limited for normal subfiles.
 */

	    if (*subname == chNull)		/* null name, ignore it */
		continue;

	    cp	  = subname;
	    cpend = cp + MYDIRSIZ;

	    while ((notsamedir || (cp < cpend)) && NOTSEP (cp))
		cp++;

/*
 * MARK END OF SUBNAME AND RETURN IT:
 */

	    *cp	      = chNull;
	    *receivep = receive;
	    *samedirp = (! notsamedir);
	    return (subname);

	} /* while */

	return (sbNull);			/* none found */

} /* GetSubName */


/************************************************************************
 * P R I N T   F I L E
 *
 * Print one filename with proper formatting.  type tells if a new line
 * must be started (NEWTRANS), additional lines must be specially indented
 * (SUBFILE), or ordinary printing is needed (ORDINARY).  missing says to
 * mark the file as missing.  Only starts a new line if necessary, and
 * always leaves nextcol somewhere on the current line.
 *
 * Note:  Prints files MINSIZE wide, just cause it looks better, even
 * though they could be as little as MYDIRSIZ (not counting indentation).
 */

proc PrintFile (type, missing, filename)
	int	type;			/* see above		*/
	int	missing;		/* mark file missing?	*/
	char	*filename;		/* name to print	*/
{
register int	space;			/* leading spaces used	*/

/* 
 * PRINT NEW LINE IF NEEDED:
 */

	if (((type == NEWTRANS) && (nextcol > 1))	/* new line required */
	||  (nextcol + INDFILE + MINSIZE > PRINTLIM))	/* line out of room  */
	{
	    printf ("\n");
	    nextcol = 1;
	}

/*
 * FIGURE SPACING AND PRINT FILENAME, with "missing" flag if needed:
 *
 * Always indent INDFILE.  For new lines, also indent INDLINE or INDSUB, as
 * required.  Actual spacing is one less to allow for "missing" flag.  No
 * need to set precision as filename is already truncated.
 */

	space = ((nextcol > 1) ? 0 : ((type == SUBFILE) ? INDSUB : INDLINE))
		+ INDFILE;

	printf ("%*s%c%-*s", space - 1, "", (missing ? MISSING : ' '),
		MINSIZE, filename);

	nextcol += space + MINSIZE;

} /* PrintFile */


/************************************************************************
 * P R I N T   M E A N I N G   C
 *
 * Print transaction meaning (subfile information) for a "C" control file
 * directly.  Only called when meanings are required and there is no
 * remote execution subfile.
 *
 * Start a new line if beyond subfile indent.  Then print only a portion of
 * a line.  (However, it's usually enough to trigger a new line next time).
 */

proc PrintMeaningC (filep)
	FILE	*filep;			/* open control file */
{
register char	*subname;		/* subfile within trans	*/
	int	receive;		/* subfile to receive?	*/
	int	samedir;		/* subfile in same dir?	*/
	char	*user = "";		/* username in trans	*/

	/* it is typeC, and need username back */
	while ((subname = GetSubName (filep, 1, & receive, & samedir, & user))
		!= sbNull)
	{
	    if (nextcol > 1 + INDSUB)		/* need start new line */
	    {
		printf ("\n%*s", INDSUB, "");
		nextcol = 1 + INDSUB;
	    }

	    /* no need to set precision for user; already truncated */
	    /* print with same layout as in PrintMeaningDX()	    */
	    printf ("%*s%-*s %c %s", INDFILE, "", NODEUSER, user,
				     (receive ? 'R' : 'S'), subname);

	    nextcol += INDFILE + NODEUSER + 3 + strlen (subname);
	}

} /* PrintMeaningC */


/************************************************************************
 * P R I N T   M E A N I N G   D X
 *
 * Print transaction meaning using username and commandline lines from
 * either a "D*X*" subfile or a "X*X*" control file, and tell whether or
 * not anything was printed.
 *
 * Checks if subname is a remote execution file (of the form "D*X....",
 * where dots represent the sequence number), else a local execution
 * file ("X*X...."), then reads the file for meanings.
 *
 * Non-matching files are ignored.  If a "D*X*" subfile is missing, the
 * usual missing-file format is used to print its name, instead of showing
 * meanings.  Otherwise, data in the subfile is used to print one line.
 *
 * Starts a new line if beyond subfile indent.  Then prints only a portion
 * of a line.  (However, it's usually enough to trigger a new line next time).
 */

proc int PrintMeaningDX (filename)
register char	*filename;	     /* "D" subfile or "X" control file	*/
{
	int	filelen;			/* length of filename	*/
	char	fileline [BUFSIZ];		/* line from file	*/
	char	nodeuser[NODEUSER+1];		/* node!user + null	*/
	char	cmdline [CMDSIZE +1];		/* commandline + null	*/

/*
 * CHECK IF EXECUTE FILE; if not, do nothing:
 */

	if ((strncmp (filename, "D.", 2)	/* not a "D." file  */
	  && strncmp (filename, "X.", 2))	/* nor a "X." file  */
	||  ((filelen = strlen (filename)) <= 4)  /* name too short */
	||  (filename [filelen - 5] != 'X'))	/* not an exec file */
	{
	    return (0);				/* nothing printed */
	}

/*
 * OPEN FILE USING STDIN; if fails, print as if missing ("D" file only):
 */

	if (freopen (filename, "r", stdin) == fileNull)
	{
	    if (*filename == 'D')
		PrintFile (SUBFILE, 1, filename);
	    return (0);				/* nothing useful printed */
	}

/*
 * GET DATA FROM SUBFILE, THEN PRINT IT:
 */

	else
	{
	    while (gets (fileline) != sbNull)	/* read all, take last found */
		GetMeaning (fileline, nodeuser, cmdline);

	    if (nextcol > 1 + INDSUB)		/* need start new line */
	    {
		printf ((catgets(nlmsg_fd,NL_SETN,874, "\n%*s")), INDSUB, "");
		nextcol = 1 + INDSUB;
	    }

	    /* no need to set precision for nodeuser; already truncated	*/
	    /* print with same layout as in PrintMeaningC()		*/
	    printf ((catgets(nlmsg_fd,NL_SETN,875, "%*s%-*s %s")), INDFILE, "", NODEUSER, nodeuser, cmdline);

	    nextcol += INDFILE + NODEUSER + 1 + strlen (cmdline);
	}

	return (1);

} /* PrintMeaningDX */


/************************************************************************
 * G E T   M E A N I N G
 *
 * Get transaction meaning information from an execution subfile line,
 * and return it in caller's buffers.
 *
 * Node!user (nodeuser) is built from "U" lines, and cmdline from "C"
 * lines, containing blank-separated fields:
 *
 *	U <username> <nodename>
 *	C <command line>
 *
 * Other line types are ignored.
 */

proc GetMeaning (subline, nodeuser, cmdline)
	char	*subline;			/* line to read		*/
	char	*nodeuser;			/* field to update	*/
	char	*cmdline;			/* field to update	*/
{
register char	*user = subline + 2;		/* start of username	*/
register char	*node = "";			/* nodename, default	*/
register char	*cp;				/* for random use	*/

/*
 * FIND NODE!USER:
 */

	if (strncmp (subline, "U ", 2) == 0)
	{
	    while (SEP (user))			/* find start of user */
		user++;

	    for (cp = user; NOTSEP (cp); cp++)	/* skip to end */
		;
	
	    if (*cp != chNull)			/* node follows */
	    {
		*cp++ = chNull;			/* mark and skip end */

		for (node = cp; SEP (node); node++)
		    ;				/* find start of node */

		for (cp = node; NOTSEP (cp); cp++)
		    ;				/* skip to end */

		*cp = chNull;			/* mark end */
	    }

	    strncpy (nodeuser, node, NODESIZE);	/* build <node>!<user> */
	    nodeuser [NODESIZE] = chNull;
	    strcat  (nodeuser, "!");
	    strncat (nodeuser, user, USERSIZE);
	}

/*
 * FIND COMMANDLINE:
 */

	else if (strncmp (subline, "C ", 2) == 0)
	{
	    for (cp = subline + 2; SEP (cp); cp++);
		;

	    strncpy (cmdline, cp, CMDSIZE);
	    cmdline [CMDSIZE] = chNull;		/* mark end */
	}

} /* GetMeaning */


/************************************************************************
 * P R I N T   S I Z E
 *
 * Print transaction size with proper formatting.  Starts a new line if
 * if the current line doesn't have enough space remaining.   Always
 * finishes the line afterwards.
 */

proc PrintSize (size)
register long	size;				/* number of bytes */
{
	char	sizestr [20];			/* number as string */

/*
 * CONVERT SIZE TO STRING:
 * 
 * Optionally round to kbytes, then see if there is enough space left on
 * this line, including some spacing:
 */

#define	KSIZE	(kflag ? ((size + 512) / 1024) : size)

	if (INDFILE + sprintf (sizestr, "%ld", KSIZE) > PRINTLIM - nextcol)
	{					/* start new line */
	    printf ((catgets(nlmsg_fd,NL_SETN,876, "\n")));
	    nextcol = 1;			/* need it in a minute */
	}

/*
 * PRINT SIZE RIGHT JUSTIFIED ON LINE:
 */

	printf ((catgets(nlmsg_fd,NL_SETN,877, "%*s\n")), PRINTLIM - nextcol, sizestr);
	nextcol = 1;

} /* PrintSize */


/************************************************************************
 * P R I N T   L I N E
 *
 * Print one line, starting at column 1, ending with newline.
 */

proc PrintLine (line)
	char	*line;			/* what to print, no newline in it */
{
	printf ("%s%s\n", ((nextcol > 1) ? "\n": ""), line);
	nextcol = 1;
} /* PrintLine */

#ifdef UCB
/* note: uses index(), not strchr(), in this code */

/************************************************************************
 * G E T   O P T
 *
 * This version of getopt(3) was written from scratch without reference
 * to the source for the Bell version.  It should be wholly compatible
 * with that version.  It is provided so this program is more portable.
 *
 * Global variables below survive across calls of this routine.
 */

int	optind = 1;				/* which option	*/
int	optoff = 0;				/* which letter	*/
char	*optarg;				/* option arg	*/
int	opterr = 1;				/* tell errors?	*/

char	*index();				/* assume this, not strchr */

#define	BADOPT	('?')				/* return for bad option */

proc getopt (argc, argv, options)
	int	argc;				/* unmodified	*/
register char	**argv;				/* unmodified	*/
register char	*options;			/* legal list	*/
{
register char	letter;				/* current one	*/

	while (optind < argc)			/* more args in arglist */
	{

/*
 * START OF ARGUMENT, check type:
 */

	    if (optoff == 0)
	    {
		if (strcmp (argv [optind], "--") == 0)	/* last option */
		{
		    optind++;			/* skip it */
		    return (EOF);
		}

		if (argv [optind] [0] != '-')	/* not option arg */
		    return (EOF);
		else
		    optoff++;			/* skip to next char */
	    }

/*
 * MORE IN ARGUMENT?  If so, is next char a valid option?
 */

	    if ((letter = argv [optind] [optoff++]) != '\0')
	    {
		if ((options = index (options, letter)) == (char *) NULL)
		{
		    if (opterr)
			fprintf (stderr, (catgets(nlmsg_fd,NL_SETN,880, "%s: illegal option -- %c\n")),
				argv[0], letter);
		    letter = BADOPT;		/* not valid option */
		}
		else

/*
 * OPTION ARG EXPECTED?  If so, use rest of this arg?
 * And if no more to this arg, is there another?
 */

		if ((options [1] == ':')	/* option arg expected */
		&&  (*(optarg = argv [optind] + optoff) == '\0'))
		{				/* no more to this arg */
		    if (++optind >= argc)	/* no more arguments   */
		    {
			if (opterr)
			    fprintf (stderr,
				    (catgets(nlmsg_fd,NL_SETN,881, "%s: option requires an argument -- %c\n")),
				    argv[0], letter);
			letter = BADOPT;
		    }
		    else				/* use next arg */
			optarg = argv [optind++];	/* and skip it  */

		    optoff = 0;
		}

		return (letter);

	    } /* if */

/*
 * ADVANCE TO NEXT ARG:
 */

	    optind++;
	    optoff = 0;

	} /* while */

	return (EOF);				/* no more arguments */

} /* getopt */

#endif /* UCB */


#ifdef BSEARCH

/************************************************************************
 * B S E A R C H
 *
 * This version of bsearch(3c) was written from scratch without reference
 * to the source for the Bell version.  It should be wholly compatible
 * with that version.  It is provided so this program is more portable.
 *
 * The manual entry doesn't say what bsearch(3c) does when there is more
 * than one matching element.  This version returns a pointer to the first
 * element that matches.
 */

proc char * bsearch (key, base, nel, size, compare)
register char	*key;			/* search key		*/
register char	*base;			/* start of data	*/
register long	nel;			/* number of elements	*/
register long	size;			/* bytes per element	*/
	int	(*compare)();		/* comparison routine	*/
{
register int	match;			/* results of compare   */
register int	hit = 0;		/* any match seen?	*/
register char	*center;		/* center position	*/
register long	halfnel;		/* half number of elems */

	while (nel > 0)				/* not exhausted yet */
	{
	    halfnel = nel / 2;			/* truncates if nel is odd */
	    center  = base + (size * halfnel);	/* right of center if even */
	    match   = (*compare) (center, key);

	    if (match >= 0)			/* center equal or too high */
	    {
		hit = (match == 0);		/* one match found? */
		nel = halfnel;			/* use lower half   */
	    }
	    else				/* center too low */
	    {
		base = center + size;		/* use upper half */
		/* to skip tested entry if nel is even */
		nel  = (nel - 1) / 2;
	    }
	} /* while */

	/* now base points to first matching entry, if any */

	return (hit ? base : (char *) NULL);

} /* bsearch */

#endif /* BSEARCH */
