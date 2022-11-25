/* HPUX_ID: @(#) $Revision: 72.1 $  */
/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : chgreel.c 
 *	Purpose ............... : Routines pertaining to media changes. 
 *	Author ................ : David Williams. 
 *	Data Files Accessed ... : 
 *	Trace Flags ........... : 
 *
 *	Description:
 *
 *	This file has all the routines used by ftio at change reel
 *	time. chgreel() is called by the tapereader/writer processes
 *	after a read/write error is encountered. It is entirely up
 *	to the code here to handle that error. Typically this means
 *	that the media is changed, and control is returned to the 
 *	caller.
 *
 *	Contents:
 *		chgreel()
 *
 *
 *-----------------------------------------------------------------------------
 */

#include "ftio.h"
#include "define.h"

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : chgreel()
 *	Purpose ............... : tape change routines entry point. 
 *
 *	Description:
 *
 *
 * 	This procedure will (optionally) invoke the tape change command 
 *	("-C command"), close the device, enter a dialog with the user
 *	to mount the next tape and check it's validity for writing,
 * 	sequence, etc.. 
 *
 *	Once the new tape has been successfully loaded, the tape 
 *	header has been written, the routine returns with a the
 *	new media's file desriptor.
 *
 *	Returns:
 *
 *	file descriptor of new media. 
 *
 */

chgreel(tape_fd, mode, tapeno)
int	tape_fd;	/* file descriptor of tape drive */
int	mode;		/* 0 is read, 1 is write */
int	tapeno;		/* no of tape I am expecting to be mounted */
{
	/*
	 *	If we are are being called for the first time,
	 *	we don't need to close the device - we just need to make 
	 *	sure we can access it.
	 *
	 *	We call it tapeno = 0, because we need to be able to do
	 *	a tapeno = 1 for bad tape restart.
	 */
	if (tapeno < 1)
	{
		tapeno++;
		goto first_time;
	}

	/*
 	 * 	If the "-S" option has been used,  invoke the  specified
	 *	command.
	*/
	if  (Change_name  != NULL) 
	{  
		sprintf(Errmsg, "%s %d %s %s", 
			Change_name, tapeno, User_name, Myname
		);

		/*
		 * 	Invoke "-S command" command
		 *
		 * 	We don't abort if this fails, because it is not
		 * 	important to the backup.
		 */
		(void)sigcld_off();
		if (system(Errmsg) < 0)
			(void) ftio_mesg(FM_SYSFL);

		(void)sigcld_on();
	}

tryagain:
	/*
	 * 	We are all done with this tape, close the device.
	 */
	rmt_close(tape_fd);

	(void) ftio_mesg(FM_TAPEN, tapeno);

	/*
	 * 	Now try to open the device 
	 */
	if ( (tape_fd = rmt_open(Output_dev_name, mode? 2: 0)) < 0 )
	{
		(void) ftio_mesg(FM_NOPEN, Output_dev_name);
		goto tryagain;
	}

first_time:
	/*
	 * 	Check if we are able to access the device!
	 */
	if ((mode == OUTPUT && (tape_fd = writetapeheader(tape_fd,tapeno)) < 0)
		||
	    (mode == INPUT && readtapeheader(tape_fd, tapeno) < 0)
	)
		goto tryagain;

	/*
	 *	ONly print the continuing message on the > 1st tapes.
	 */
	if (tapeno > 1 || Filelist)
		(void)ftio_mesg(FM_CONT);

	return(tape_fd);
}

/*
 *	The following header is a static which holds the 'master'
 *	tape header.
 */
static	struct	ftio_t_hdr t_hdr;

#define GOOD_HEADER     0
#define WRONG_BACKUP    2
#define WRONG_MEDIA     1
#define	WRONG_CHECK	-2

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : Writetapeheader()
 *	Purpose ............... : Writes out a tape header.
 *
 *	Description:
 *		Writetapeheader tests if the device is writeable by
 *		writing writing a ftio header to it. If this fails
 *		writetapeheader() checks if the write ring is in
 *		place and prints a diagnostic if it is not.
 *
 *	Returns:
 *		Writetapeheader returns the device file descriptor 
 *		if all is ok, else it closes the file and returns -1.
 */

writetapeheader(fd, no)
int	fd;
int	no;
{
	struct	ftio_t_hdr buffer;
	int	use_count = 1;
	int	list_fd;

	/* 
	 *	Only write headers to a tape 
	 */
	if (!Tape_headers)	
		return fd;

	/*
	 *	Read in enough to check out the header.
	 *
	 *	Note if the read fails, we just blast over
	 *	it! - Else we could not handle brand new tapes.
	 */
	if (rmt_read(fd, &buffer, sizeof(buffer)) != -1)
	{
		/*
		 *	See if it is an ftio tape and
		 *	it is not the tape we just wrote out...
		 */
		switch(examinetapeheader(&buffer, no))
		{
		case WRONG_MEDIA:	/* not expecting this media no */ 
			if (no > 1 && !ftio_mesg(FM_WRGMEDIA)) 
			{
				(void)rmt_close(fd);
				return -1;
			}
			/* else */
			/*FALL THROUGH*/

		case GOOD_HEADER:/* the header we are expecting (never this) */
		case WRONG_BACKUP:	/* another FTIO backup */
			/*
			 *	The Dan Osecky memorial feature..
			 *
			 *	Check to see how many times the tape has been 
			 *	used
			 */
			use_count = atoi(buffer.tapeuse) + 1;
			if (use_count > 100)
			{
				if (!ftio_mesg(FM_TAPEUSE))
				{
					(void)rmt_close(fd);
					return -1;
				}
			}

			/*
			 *	Test if user is going to write over another's
			 *	backup.	Give a warning if the user is about
			 *	to trash someone else's tape.
			 */
			if (User_id && strcmp(buffer.username, User_name))
			{
				if (!ftio_mesg(FM_OTAPE))
					return -1;
			}
			break;

		default:		/* new media, not FTIO, etc, etc */	
			break;
		}
	}

	/*
	 * 	Now try to re-open the device and continue.
	 */
	(void)rmt_close(fd);
	if ((fd = rmt_open(Output_dev_name, 2)) < 0)
	{
		(void) ftio_mesg(FM_NOPEN, Output_dev_name);
		return -1;
	}

	/*
	 * 	Create a tape header.
	 */
	createheader(&buffer, no, use_count);

	if ((rmt_write(fd, &buffer, sizeof(buffer))) < sizeof(struct ftio_t_hdr))
	{
		(void)ftio_mesg(FM_WTHDR, Output_dev_name);
		(void)rmt_close(fd);
		return(-1);
	}

	/*
	 *	Copy file list onto tape, if we have that option on.
	 */
	if (Filelist)
	{
#ifdef	DEBUG
		if (Diagnostic)
			fprintf(stderr, "XXX: copying filelist onto tape\n");
#endif	DEBUG
		if ((list_fd = open(Filelist, 0)) == -1)
		{
			(void)ftio_mesg(FM_NOPEN, Filelist);
			return -1;
		}

		if (copy(fd, list_fd) == -1)
		{
			(void)close(list_fd);
			(void)rmt_close(fd);
			return -1;
		}
		(void)close(list_fd);
	}

	/*
	 * 	Write EOF marker..
	 */
	write_eof(fd);
	return fd;
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : readtapeheader()
 *	Purpose ............... : Reads in a tape header. 
 *
 *	Description:
 *		Readtapeheader attempts to read a tape header at
 *		the current file position.
 *
 *	Returns:
 *
 *		 0 if success. 
 *		-1 if fail.
 */

readtapeheader(fd, no)
int	fd;
int	no;
{
	struct	ftio_t_hdr buffer;
	int	listed = 0;
	int	early_exit = 0;

	/* 
	 *	Only read headers from tape 
	 */
	if (!Tape_headers)	
	{
		Tape_no = no;
		return 0;
	}

	/*
	 *	Read in the header.
	 */
	if (rmt_read(fd, &buffer, sizeof(buffer)) == -1)
	{
		ftio_mesg(FM_RTHDR);
		return(-1);
	}

	/*
	 *	Examine it for sequence, etc.
	 */
	switch(examinetapeheader(&buffer, no))
	{
	case GOOD_HEADER:
		if (Listfiles)
		{
			(void)look_header(&buffer, no);
			listed++;
		}
		break;

	case WRONG_MEDIA:
		if (!listed)
		{
			(void)look_header(&buffer, no);
			listed++;
		}

		if (ftio_mesg(FM_WRGMEDIA))
			break;
		else
			return -1;

	case WRONG_BACKUP:
		if (!listed)
		{
			(void)look_header(&buffer, no);
			listed++;
		}

		if (ftio_mesg(FM_WRGBCKUP))
		{
			/*
			 *	If we are starting on the non-first
			 *	tape, then indicate to the file writing
			 *	process the offset to the first file.
			 */
			Packets[0].offset = atoi(buffer.offset);
			Packets[0].status = PKT_SKIP;
			break;
		}
		else
			return -1;

		/*
		 *	Examineheader() has identified the tape as a bad 
		 *	one.
		 */
	case WRONG_CHECK:
		(void) ftio_mesg(FM_HDRCHECK);
		if (!Diagnostic)
			return -1;

		if (ftio_mesg(FM_QUESTION, "continue? "))
		{
			early_exit++;
			break;
		}
		else
			return -1;

	default:
		(void) ftio_mesg(FM_HDRMAGIC);
		if (!Diagnostic)
			return -1;
		
		if (ftio_mesg(FM_QUESTION, "continue? "))
		{
			early_exit++;
			break;
		}
		else
			return -1;
	}

	/*
	 * 	Now skip over EOF marker
	 */
	if (skip_filelist(fd) == -1)
	{
		ftio_mesg(FM_NREOF);
		return -1;
	}

	/*
	 *	Only for detected bad tapes, that we want to read anyway..
	 */
	if (early_exit)
		return 0;
	
	/*
	 *	Update tape number..
	 */
	Tape_no = atoi(buffer.tapeno + 1);

	/*
	 *	Update tape parameters:
	 */
	if (no < 2)
	{
		int	htype;
		int	bsize;

		bsize = atoi(buffer.blocksize);
		if (Blocksize != bsize)
		{
			if (!listed)
			{
				(void)look_header(&buffer, no);
				listed++;
			}
			(void)ftio_mesg(FM_WRGBLKSIZ);
			Blocksize = bsize;
		}

		if (*buffer.headertype == 'c')
			htype = 1;
		else
			htype = 0;
		
		if (htype != Htype)
		{
			if (!listed)
			{
				(void)look_header(&buffer, no);
			}
			(void)ftio_mesg(FM_WRGHTYPE);
			Htype = htype;
		}
	}

	return 0;
}

#define	FTIO_MAGIC "FTIO LABEL"	


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : createheader()
 *	Purpose ............... : Form tape header in memory. 
 *
 *	Description:
 *
 *	Creates a tape header from the master header held as a static 
 *	by adding tape number, and checksum information.
 *
 *	Returns:
 *
 *	none 
 *
 */

createheader(s, no, use)
struct	ftio_t_hdr *s;
int	no;
int	use;
{
	int	check_sum = 0;
	char	*p;

	char	*nl_ctime();

	/* 
	 * 	For the first tape only,
	 * 	create the whole structure.
	 */
	if (no < 2)
	{
		struct	utsname	name;
		struct	timeval	timevalue;
		struct	timezone timez;	

		/*
		 * 	Firstly PAD out target memory 
		 * 	use spaces, then null terminate each field.
		 * 	- making it readable for cat /dev/rmt..
		 */
		(void)memset(&t_hdr, ' ', sizeof(struct ftio_t_hdr));

		/*
		 * 	Get uname(2) info
		 */
		if (uname(&name) < 0)
		{
			(void)ftio_mesg(FM_NUNAME);
		}

		/*
		 * 	Put magic number in
		 */
		(void)strcpy( t_hdr.magic, FTIO_MAGIC );

		/*
		 * 	Copy over uname(2) items
		 */
		(void)strcpy( t_hdr.machine, name.machine );
		(void)strcpy( t_hdr.sysname, name.sysname );
		(void)strcpy( t_hdr.release, name.release );
		(void)strcpy( t_hdr.nodename, name.nodename );
		
		/*
		 * 	Put in username
		 */
		(void)strcpy(t_hdr.username, User_name);

		/*
		 * 	Blocksize:
		 */
		(void)sprintf(t_hdr.blocksize, "%05d", Blocksize);

		/*
		 * 	Write in the headertype:
		 */
		*t_hdr.headertype = (Htype? 'c': 'b');
		*(t_hdr.headertype + 1) = '\0';

		/* 
		 * 	Get time and date
		 */
		if (gettimeofday(&timevalue, &timez) == -1)
		{
			(void)ftio_mesg(FM_NGETT);
			timevalue.tv_sec = 0;
		}

		/*
		 * 	Place the `date` into header..
		 */
		(void)strcpy(t_hdr.time, 
			nl_ctime((long *)&timevalue.tv_sec, 
				 "%H:%M:%S %D", currlangid() 
				) 
		      );
		
		/*
		 *	Remove trailing '\n'
		 */
		if (p = strchr(t_hdr.time, '\n'))
		{
			*p = '\0';
			*(p + 1) = ' ';
		}

		/*
		 * 	Add the comment field passed from the 
		 *	command line (-K "comment").
		 */
		(void)strncpy(t_hdr.comment,Komment,sizeof(t_hdr.comment) - 1);
	}

	/*
	 * 	Copy over initial header.
	 */
	(void)memcpy(s, &t_hdr, sizeof(struct ftio_t_hdr));
	
	/*
	 * 	Write in the tape usage count.
	 */
	(void)sprintf(s->tapeuse, "%04d", use);

	/*
	 * 	Put in tape #
	 */
	(void)sprintf(s->tapeno, "#%.3d", no);

	/*
	 *	Insert offset to first file on tape.
	 *	..later.
	sprintf(s->offset, "%.11u", 
	 */

	/* 
	 * 	Finally calculate and insert checksum:
	 */
	for (p = (char *)s; p < (char *)s->check; p++)
		check_sum += *p;
	
	(void)sprintf(s->check, "%.3u", check_sum % 256);
}


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : examinetapeheader()
 *	Purpose ............... : Examines the tape header structure in *s. 
 *
 *	Description:
 *
 *	
 *
 *	Returns:
 *
 *	-1	-	bad header, or not ftio.
 *	WRONG_CHECK -	check sum incorrect
 *	GOOD_HEADER -	got the header with the required tape no.
 *	WRONG_MEDIA -	correct backup set, but wrong media number.
 *	WRONG_BACKUP -	from another ftio backup.
 */

examinetapeheader(s, no)
struct	ftio_t_hdr *s;
int	no;
{
	int	check_sum = 0;
	char 	*p;


	/*
	 *	Check for ftio magic number.
	 */
	if (strcmp(s->magic, FTIO_MAGIC))
		return -1;

	/*
	 *	We think it is FTIO format, check checksum.
	 */
	for (p = (char *)s; p < (char *)s->check; p++)
		check_sum += *p;

	if ((check_sum % 256) != atoi(s->check))
		return WRONG_CHECK;
	
	/*
	 *	If it is the first tape, copy over the header into
	 *	static t_hdr space.
	 *
	 *	Else check to see if we have the correct backup set.
	 */
	if (no == 1)
		(void)memcpy(&t_hdr, s, sizeof(t_hdr));
	else
		if (
			(int)strcmp(s->nodename, t_hdr.nodename)
			||
			(int)strcmp(s->time, t_hdr.time)
		)
			return WRONG_BACKUP;

	/*
	 *	Ok it's the same backup, no check the media number.
	 */
	if (no == atoi(s->tapeno + 1))
		return GOOD_HEADER;
	else
		return WRONG_MEDIA;
}
 

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : look_header()
 *	Purpose ............... : Print ftio tape header in formatted way. 
 *
 *	Description:
 *
 *	
 *
 *	Returns:
 *
 *	none. 
 *
 */

look_header(s, no)
struct	ftio_t_hdr *s;
int	no;
{
	printf("\
Tapeheader for tape no %d:\n\n\
Nodename: %-8s     Creation time: %s     Creator: %-s\n\
Comment: %s\n\
Tape #: %-5s          Tape usage #: %s\n",
		no, 
		s->nodename,
		s->time, 
		s->username,
		s->comment,
		s->tapeno, 
		s->tapeuse
	);

	/*
	 *	Only print OS release, etc for first tape.
	 */
	if (no > 1)
		return;

	printf("\n\
Machine type: %-8s Unix flavor: %-8s OS Release #: %s\n\
Blocksize   : %-8s Header type : %s\n\n",
		s->machine,
		s->sysname,
		s->release,
		s->blocksize,
		s->headertype
	);
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : copy()
 *	Purpose ............... : copy a file to another. 
 *
 *	Description:
 *
 *	Copy contents of file descriptor fda to file descriptor fdb,
 *	blocked to Blocksize blocks, the last block is null padded.
 *
 *	Returns:
 *
 *	-1 	for fail 
 *	0 	for success.	
 *
 */

copy(fdb, fda)
int	fdb,
	fda;
{
	char	*p,
		*malloc();
	int	retval;

	if ((p = malloc(Blocksize)) == NULL)
	{
		(void)ftio_mesg(FM_NMALL);
		return -1;
	}

	while ((retval = read(fda, p, Blocksize)) != 0)
	{
		if (retval == -1)
		{
			(void)ftio_mesg(FM_NREAD, Filelist);
			return -1;
		}

		if (retval < (int)Blocksize)
			(void)memset(p+retval, (int)0, (int)Blocksize - retval);

		if (rmt_write(fdb, p, Blocksize) == -1)
		{
			(void)ftio_mesg(FM_NWRIT, Output_dev_name);
			return -1;
		}
	}

	(void)free(p);

	return 0;
}


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : skip_filelist()
 *	Purpose ............... : Skip over the tape filelist. 
 *
 *	Description:
 *
 *	Reads Blocksize blocks until an EOF is encountered. 
 *
 *	Returns:
 *
 *	-1 for fail. 
 *
 */

skip_filelist(fd)
int	fd;
{
	char	*p,
		*malloc();
	int	retval;

	if ((p = malloc(Blocksize)) == NULL)
	{
		(void)ftio_mesg(FM_NMALL);
		return -1;
	}

	while ((retval = rmt_read(fd, p, Blocksize)) != 0)
		if (retval == -1)
			return -1;

	(void)free(p);

	return 0;
}
