/* $Revision: 64.2 $ */
/*
 * Check revision information for lists of files.
 * Compile with -DDEBUG for more output.
 *
 * For each argument (reference file), this program does the following:
 *
 * 	- Get a reference buffer consisting of a line that starts with a
 *	  filename, followed by zero or more lines of whatstrings, up till
 *        EOF or the next filename line (but only the first BUFSIZE bytes are
 *	  actually compared).
 * 
 *	- Extract from the reference buffer the filename to check, and at the
 *	  same time set up a command line for popen() such that COMMAND is
 *	  executed on the filename.
 *
 *	- Execute the command line and read the results back into a current
 *	  buffer, the same way the reference buffer was built.
 *
 *	- Compare the reference and current buffers and report if different.
 */


#include <stdio.h>

#define COMMAND "what"               /* command that returns rev info    */
#define CMDSIZ  10                   /* to allow for cmd name plus blank */
#define BUFSIZE 1024

char	*myname;			/* name invoked as */


revck (argc, argv)
	int	argc;
	char	**argv;
{
	FILE	*rfilep;		/* currently open ref_file */
	int	mismatch = 0;		/* flag a mismatch for exit status */

/*
 * Check arguments:
 */
	myname = *argv;

	if (argc < 2) {
		fprintf (stderr, "usage: %s ref_files\n", myname);
		exit (1);
	}
/*
 * Do all ref_files one at a time:
 */
	while (*(++argv))
		if ((rfilep = fopen (*argv, "r")) == NULL)
			fprintf (stderr, "%s: can't open %s\n", myname, *argv);
		else {
			mismatch += doreffile (rfilep);
			fclose	  (rfilep);
		}
	if (mismatch)
		exit (1);
	else
		exit (0);
}


/*****	*****	*****	*****	*****	*****	*****	*****	*****
 * Do one reference file:
 */

int doreffile (rfilep)
	FILE	*rfilep;
{
/* getfilename() assumes filename[] & cmdbuf[] aren't smaller than refbuf[] */

	char    refbuf  [BUFSIZE];        /* reference buffer from ref_file   */
	char    currbuf [BUFSIZE];        /* new buffer returned from command */
	char    filename[BUFSIZE];        /* file being checked               */
	char    cmdbuf  [CMDSIZ+BUFSIZE]; /* command line for popen()         */
	int	misflag = 0;		  /* flag a revision mismatch         */
	FILE    *cfilep;                  /* currently open command pipe      */
	FILE	*popen();

/*
 * Get each reference buffer (filename line and whatstring lines):
 */
	while (getbuffer (refbuf, rfilep)) {

/*
 * Extract filename and command buffer from reference buffer:
 */
		getfilename (filename, cmdbuf, refbuf);

#ifdef DEBUG
		printf ("r: %sf: %s\nc: %s\n", refbuf, filename, cmdbuf);
#endif

/*
 * Execute command attached to pipe and get current buffer:
 */
		if ((cfilep = popen (cmdbuf, "r")) == NULL) {
			fprintf (stderr, "%s: can't exec '%s'\n",
				myname, cmdbuf);
			exit (1);
		} else {
			getbuffer (currbuf, cfilep);
#ifdef DEBUG
			printf ("c: %s\n", currbuf);
#endif
			pclose (cfilep);

/*
 * Compare reference and current buffers:
 */
			if (strcmp (refbuf, currbuf)) {
				fprintf (stderr, "%s: revision mismatch: %s\n",
						myname, filename);
				misflag++;
			}
		}
	}
	return (misflag);
}


/*****	*****	*****	*****	*****	*****	*****	*****	*****
 * Get buffer (file name and rev number lines) from a stream:
 * Null line (two newlines in a row) is treated as a whatstring.
 * Sinks characters beyond BUFSIZE.
 * Always returns buffer with a null terminator.
 */

getbuffer (bufp, filep)
	char	*bufp;
	FILE	*filep;
{
	int	nlflag = 0;			/* just passed a newline */
	long	count  = 0;			/* chars read so far	 */
	
	while ((*bufp = getc (filep)) != EOF)	/* stop at end of file	   */
	{
		if (nlflag && (*bufp != '\t') && (*bufp != '\n'))
		{				/* start of new filename   */
			ungetc (*bufp, filep);	/* save first char of name */
			*bufp = 0;		/* trailing null	   */
			return (1);		/* buffer is ready	   */
		}
		nlflag = (*bufp == '\n');	/* note if at newline	   */
		bufp  += (++count < BUFSIZE);   /* don't overflow buffer   */
	}
	*bufp = 0;				/* trailing null	   */
	return (count > 0);			/* buffer could end at EOF */
}


/*****	*****	*****	*****	*****	*****	*****	*****	*****
 * Get filename and command buffer from reference buffer:
 * Copies up to null or newline, then puts a trailing null over the last
 * character (assumed to be ":" returned from COMMAND).  Assumes that rbufp
 * does not start with a newline.
 */

getfilename (fnamep, cbufp, rbufp)
	char	*fnamep, *cbufp, *rbufp;
{
	char	*cp = COMMAND;			/* command name */

	while (*cp)				/* start with command name */
		*cbufp++ = *cp++;
	*cbufp++ = ' ';				/* append blank sep */

	while (*rbufp && (*rbufp != '\n'))	/* up to null or newline */
		*fnamep++ = *cbufp++ = *rbufp++;

	*(--fnamep) = 0;			/* trailing nulls */
	*(--cbufp)  = 0;
}
