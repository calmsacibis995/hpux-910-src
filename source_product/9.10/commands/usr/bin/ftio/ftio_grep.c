/* HPUX_ID: @(#) $Revision: 64.1 $  */


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : ftio_grep 
 *	Purpose ............... : Print file list from ftio tape. 
 *	Author ................ : David Williams. 
 *
 *	Description:
 *
 *	These routines allow the user to list what is on an ftio tape
 *	that has been created with the -L option. Patterns may be specified
 *	to match names in the filelist. These patterns follow the
 *	same rules as all patterns in ftio/cpio.
 *
 *	The code reads the header and automatically sets Blocksize and
 *	headertype. if -v was specified, the tape header is printed as well.
 *
 *	Contents:
 *
 *		ftio_grep()
 *		char *get_line();
 *		name_match()
 *
 *-----------------------------------------------------------------------------
 */

#include	"ftio.h"
#include	"ftio_mesg.h"

char	*block_at;

ftio_grep(argc, argv)
int	argc;
char	**argv;
{
	int	fd;
	int	tapeno;
	struct	ftio_t_hdr buffer;
	char	*malloc();
	char	*get_line();

	/*
	 *	Check argc..
	 */
	if (argc < 3)
	{
		(void)ftio_mesg(FM_ARGC);
		(void)exit(1);
	}

	/*
	 *	Check for list files option.
	 */
	if (strchr(argv[1], 'v'))
		Listfiles = 1;

	/*
	 *	Usage: ftio -g /dev/rmt/0h patterns.. ..
	 */
	if ((fd = rmt_open(argv[2], 0)) == -1)
	{
		(void)ftio_mesg(FM_NOPEN, argv[2]);
		(void)exit(1);
	}

	/*
	 *	We can't use readtape - it has all that awful
	 *	user - friendly interaction stuff.... yuk!
	 */
	if (rmt_read(fd, &buffer, sizeof(buffer)) == -1)
	{
		(void)ftio_mesg(FM_RTHDR, argv[2]);
		(void)rmt_close(fd);
		(void)exit(1);
	}

	/*
	 *	Use examineheader() to make sure it is an ftio tape.
	 */
	if (examinetapeheader(&buffer, 1) == -1)
	{
		(void)ftio_mesg(FM_RTHDR, argv[2]);
		(void)rmt_close(fd);
		(void)exit(1);
	}

	/*
	 *	Set Blocksize, etc..
	 */
	Blocksize = atoi(buffer.blocksize);
	tapeno = atoi(buffer.tapeno + 1);

	/*
	 *	Print the tape header..
	 */
	if (Listfiles)
	{
		(void)look_header(&buffer, tapeno);
		(void)fflush(stdout);
	}

	/*
	 *	Grab some space to work in.
	 */
	if ((block_at = malloc(Blocksize)) == NULL)
	{
		(void)ftio_mesg(FM_NMALL);
		(void)rmt_close(fd);
		(void)exit(1);
	}

	/*
	 *	Print the filelist..
	 */
	while((get_line(fd, Pathname)) != NULL)
	{
		if (argc < 4 || name_match(Pathname, &argv[3]))
			puts(Pathname);
	}

	(void)rmt_close(fd);
	(void)exit(0);
}

static	unsigned posn_in_block = 0;

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : get_line()
 *	Purpose ............... : get a line from the tape filelist. 
 *
 *	Description:
 *
 *	Just gets the next line, and returns it without a \n on end 
 *
 *	Returns:
 *
 *	pointer to line from filelist, or NULL for EOF. 
 *
 */

char	*
get_line(fd, target)
int	fd;
char	*target;
{
	char	*from;
	char	*to;
	int	readret;
	int	nulls = 0;

	/*
	 *	The first time, we must get something to play with.
	 */
	if (!posn_in_block)
	{
		if ((readret = rmt_read(fd, block_at, Blocksize)) == -1)
		{
			(void)ftio_mesg(FM_NREAD, "filelist");
			(void)rmt_close(fd);
			(void)exit(1);
		}

		if (!readret)
			return NULL;
	}

	to = target;

	/*
	 *	Loop until we have copied a string to target.
	 */
	while(1)
	{
		/*
		 *	Set pointer to current position in block.
		 */
		from = block_at + posn_in_block++;

		/*
		 *	The definition of EOF for us is
		 *	two NULLs in a row (or readret = 0)
		 *
		 *	Copy over bytes till you get a string's worth.
		 */
		switch(*from)
		{
		case '\n':
			*to = '\0';
			return target;

		case '\0':
			if (nulls)
				return NULL;
			nulls++;
			break;

		default:
			*to++ = *from;
			nulls = 0;
			break;
		}

		/*
		 *	If we are at the end of this block get a new one.
		 */
		if (posn_in_block >= Blocksize)
		{
			if ((readret = rmt_read(fd, block_at, Blocksize)) == -1)
			{
				(void)ftio_mesg(FM_NREAD, "filelist");
				(void)rmt_close(fd);
				(void)exit(1);
			}

			if (!readret)
				return NULL;

			posn_in_block = 0;
		}
	}
}


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : name_match()
 *	Purpose ............... : Match the name part of a filelist line. 
 *
 *	Description:
 *
 *	Skips over tape number in line and calls nmatch() to 
 *	attempt to match the filename.
 *
 *	Returns:
 *
 *	1 if match 0 if no. 
 *
 */

name_match(str, pat)
char	*str;
char	**pat;
{
	char	*p;

	/*
	 *	Skip over leading media number, and space.
	 */
	for (p = str; *p != ' '; p++);
	for (; *p == ' '; p++);

	/*
	 *	Return value reported by nmatch()
	 */
	return nmatch(p, pat);
}



