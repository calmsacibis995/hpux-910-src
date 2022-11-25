/* HPUX_ID: @(#) $Revision: 72.2 $  */
/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : fileloader.c 
 *	Purpose ............... : Reads files into shared memory space. 
 *	Author ................ : David Williams. 
 *
 *	Description:
 *
 *	This file contains the routines that deal with reading files 
 *	from the disk and creating a ftio/cpio format archive in memory.
 *      Fileloader() is essentially main() for this process, after the
 *	routines in main.c have sorted out the arguments and checked
 *	the validity of them.
 *
 *	Contents:
 *
 *
 *-----------------------------------------------------------------------------
 */

#include	"ftio.h"
#include	"define.h"

static	char	**ig;


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : fileloader()
 *	Purpose ............... : Load files from disk into memory. 
 *
 *	Description:
 *
 *	The argument "pathnames" is a pointer to the array of arguments
 *	which are the pathnames to backup, optionally followed by
 *	"-F" and the patterns not to back up.
 *
 *	NOTE: that if Filelist has a non-zero value, fileloader() 
 *	assumes a list of files has been created to back up, and
 *	will use this instead of file-tree-walking or reading from
 *	stdin.
 *
 *	Returns:
 *
 *	ABORT if fail. 
 *
 */

fileloader(pathnames)
char	**pathnames;
{
	int	i;
	int	call_grabber();

	char	**find_f();

	if (Filelist)
	{
		
		/*
		 *	Open filelist coming from restart list.		
		 */
		if ((Restart_fp = fopen(Restart_name, "r")) == NULL)
		{
			(void)ftio_mesg(FM_NOPEN, Restart_name);
			return ABORT;
		}

		/*
		 * 	Use getfile() to read Listfp 
		 * 	for a list of files to backup
		 */
		(void)filegrabber((char *)0, (char *)0, &Stats, START);	

		while (getfile(Restart_fp) >= 0)
		{
			/*
			 *	All files that make it onto the filelist
			 *	get shipped, call filegrabber().
			 */
			switch(filegrabber(Pathname, Pathname, &Stats, NORMAL))
			{
			case ABORT:
				return ABORT;

			case RESTART:
				if (restart_backup() == ABORT)
					return ABORT;
				File_number++;	
				break;

			default:
				File_number++;	
				break;
			}
		}

		(void)filegrabber((char *)0, (char *)0, &Stats, FINISH);

		return NORMAL;
	}
	
	/*
	 *	OK We are not using filelist, back up on the fly
	 *	- the good old way!
	 *
	 *      Separate into list of pathnames and ignorenames.
	 *	This also sets Matchpatterns and Read_stdin.
	 */
	ig = find_f(pathnames);

	/*
	 *	If we are reading a file list from stdin...
	 */
	if (Read_stdin)
	{
		/*
		 * 	Use getfile() to read Listfp 
		 * 	for a list of files to backup
		 */
		(void)filegrabber((char *)0, (char *)0, &Stats, START);	

		while (getfile(stdin) >= 0)
		{
			/*
			 *	If we are doing an incremental, check that
			 *	the file is new enough. For a full backup,
			 *	Inc_date = 0.
			 */
			if (Stats.st_mtime <= Inc_date)
				continue;

			/*
			 *	If we are "-F" pattern ignoring,
			 *	check..
			 */
			if (Matchpatterns && nmatch(Pathname, ig))
				continue;
			
			/*
			 *	Ok we are shipping this file,
			 *	send it to filegrabber().
			 */
			switch(filegrabber(Pathname, Pathname, &Stats, NORMAL))
			{
			case RESTART:
				(void)ftio_mesg(FM_UNEX, 
					"fileloader(): RESTART"
				);
				/* FALL THROUGH */

			case ABORT:
				return ABORT;

			default:
				File_number++;	
				break;
			}
		}

		(void)filegrabber((char *)0, (char *)0, &Stats, FINISH);
	}
	else	/* searching down files.. */
	{
		/*
		 * 	Use search() to descend the directory
		 * 	specified in by Filename and output to tape
		 */

		(void)filegrabber((char *)0, (char *)0, &Stats, START);	

		for (i = 0; pathnames[i]; i++)
		{
			Filename = (char *)pathnames[i];

			/* 
			 *	Walk the 'pathname' tree looking for files.
			 *	Call_filegrabber() is used to sift out
			 *	the unwanted files - based on -N and -F.
			 */
			search(Filename, call_grabber); 
		}

		(void)filegrabber((char *)0, (char *)0, &Stats, FINISH);
	}
	return(0);
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : make_filelist()
 *	Purpose ............... : Makes a list of files to back up. 
 *
 *	Description:
 *
 *	Make_filelist() creates a list of files in a temporary file
 *	'/tmp/ftio.PID', it also copies this list to "ftio.list", 
 *	prepending "1 " to each line.
 *
 *	The argument "paths" is a pointer to the array of arguments
 *	which are the pathnames to backup, optionally followed by
 *	"-F" and the patterns not to back up.
 *
 *	Returns:
 *
 *	-1 for fail. 
 *
 */

make_filelist(paths)
char	**paths;
{
	FILE	*fp;
	char	*p;
	int	i;

	char	**find_f();
	int	namesave();

	(void)ftio_mesg(FM_FLIST);

	/*
	 * 	Seperate into list of pathnames and ignorenames.
	 *	This also sets Matchpatterns and Read_stdin.
	 */
	ig = find_f(paths);

	if ((Restart_fp = fopen(Restart_name, "w")) == NULL)
	{
		(void)ftio_mesg(FM_NOPEN, Filelist);
		return -1;
	}

	if (Read_stdin)
	{
		/*
		 *	Copy stdin to restart list.
		 */
		while (getfile(stdin) >= 0)
		{
			/*
			 *	If we are doing an incremental, check that
			 *	the file is new enough. For a full backup,
			 *	Inc_date = 0.
			 */
			if (Stats.st_mtime <= Inc_date)
				continue;

			/*
			 *	If we are "-F" pattern ignoring,
			 *	check..
			 */
			if (Matchpatterns && nmatch(Pathname, ig))
				continue;
			
			fputs(Pathname, Restart_fp);
			fputc('\n', Restart_fp);
		}
	}
	else
	{
		for (i = 0; paths[i]; i++)
		{
			Filename = paths[i];
			/* 
			 *	Walk the 'pathname' tree looking for files.
			 */
			search(Filename, namesave); 
		}

		/*
		 *	Get back into the starting directory, or we
		 *	can never find the filelist!
		 */
		if (chdir(Home) == -1)
		{
			(void)ftio_mesg(FM_NCHDIR, Home);
			return -1;
		}
	}

	(void)fclose(Restart_fp);
	
	/*
	 *	Now make first tape list.
	 */
	if ((Restart_fp = fopen(Restart_name, "r")) == NULL)
	{
		(void)ftio_mesg(FM_NOPEN, Restart_name);
		return -1;
	}

	if ((fp = fopen(Filelist, "w")) == NULL)
	{
		(void)ftio_mesg(FM_NOPEN, Filelist);
		return -1;
	}

	while (fgets(Pathname, MAXPATHLEN, Restart_fp) != NULL)
	{
		p = Pathname;
		if (*p == '.' && *(p + 1) == '/')
			p += 2;
		else if (*p == '/' && Makerelative)
			p++;

		fprintf(fp, "%d %s", 1, p);
	}
	
	(void)fclose(fp);
	(void)fclose(Restart_fp);
	return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : namesave()
 *	Purpose ............... : Save filename given in the filelist. 
 *
 *	Description:
 *
 *	A pointer to namesave() is passed to the file-tree-walking 
 *	routine. Namesave accepts the info for the file found, checks
 *	that the file is up to date enough (when doing an incremental),
 *	checks that it does not match a ignore pattern, then writes
 *	it to the (previously opened) filelist.
 *
 *	Returns:
 *
 *	none really. 
 *
 */
/*ARGSUSED*/
namesave(dummy, pathname, stats, dummy2)
char	*dummy,
	*pathname;
struct	stat	*stats;
int	dummy2;
{
	/*
	 *	If we are doing an incremental, check that
	 *	the file is new enough. For a full backup,
	 *	Inc_date = 0.
	 */
	if (stats->st_mtime <= Inc_date)
		return NORMAL;
	
	/*
	 *	If we are "-F" pattern ignoring,
	 *	check..
	 */
	if (Matchpatterns && nmatch(pathname, ig))
		return NORMAL;
	
	fputs(pathname, Restart_fp);
	fputc('\n', Restart_fp);

	return NORMAL;
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : find_f()
 *	Purpose ............... : Seperate "-F" pathnames from others. 
 *
 *	Description:
 *
 *	Find_f() examines the array of args given to it, which is 
 *	assumed to be of the form:
 *
 *		[pathnames .. ..] [-F ignorenames .. ..]
 *
 *	There are four possible results:
 *
 *	There are no pathnames and no ignorenames (no -F actually).
 *	In this case Read_stdin = 1, Matchpatterns = 0, NULL is returned.
 *
 *	There are patterns but no ignorenames. Read_stdin = 0,
 *	Matchpatterns = 0, NULL is returned.
 *	
 *	There are no pathnames, there are ignorenames. Read_stdin = 1,
 *	Matchpatterns = 1, a pointer to ignorenames is returned.
 *
 *	there are pathnames, and there are ignorenames Read_stdin = 0,
 *	Matchpatterns = 1, a pointer to ignorenames is returned.
 *
 *	Returns:
 *
 *	pointer to ignorenames[]. 
 */

char	**
find_f(patterns)
char	**patterns;
{
	char	**p;

	Read_stdin = 1;

	for (p = patterns; *p; p++)
	{
		if (!strcmp(*p, "-F"))
		{
			*p = NULL;

			if (*(++p))
			{
				Matchpatterns = 1;
				return p;
			}
		}
		else
			Read_stdin = 0;
	}
	return NULL;
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : restart_backup()
 *	Purpose ............... : Restarts the backup after a bad media fail. 
 *
 *	Description:
 *
 *	Restart_backup opens the restart list for reading, seeks the 
 *	name of the file that was being backed up at the Start of this
 *	media, calls filegrabber to do a restart from that file,
 *	then returns control to the caller, ao that the backup can 
 *	proceed from there.
 *
 *	Returns:
 *
 *	ABORT if failed. 
 *
 */

restart_backup()
{
	int	fname_offset;
	int	i;
	char	*p,
		*strchr();

	fname_offset = Packets[Nobuffers].fname_offset;

	/*
	 *	First close the restartlist
	 */
	(void)fclose(Restart_fp);
	if ((Restart_fp = fopen(Restart_name, "r")) == NULL)
	{
		ftio_mesg(FM_NOPEN, Restart_name);
		return ABORT;
	}

	/*
	 *	Seek the name of the file to restart on..
	 */
	for (i = 0; i <= fname_offset; i++)
	{
		(void)fgets(Pathname, MAXPATHLEN, Restart_fp);
		if (p = strchr(Pathname, '\n'))
			*p = '\0';
	}

	/*
	 *	Reset the number of the file we are backing up.
	 */
	File_number = fname_offset;

	/*
	 *	Redo first file (file that is over boundary).
	 */
	(void)filegrabber(Pathname, Pathname, 0, RESTART);

	return NORMAL;
}

static	char	*save_hdr = 
"THIS IS THE SAVE HEADER AREA................................................";
static	char	*cp,
		*ep;

static	int	cur_pkt;

#define	HEADER_1 0
#define	HEADER_2 1
#define	FILENAME_1 2
#define	FILENAME_2 3
#define	BODY_1	4
#define	BODY_2	5
#define	PAD	6


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : call_grabber()
 *      Purpose ............... : Used by ftw to call filegrabber().
 *
 *	Description:
 *
 *	Call_grabber() is called by the file_tree_walk algorithm,
 *	to conditionally decide if the file found should be passed
 *      to filegrabber(). Call_grabber() checks if the file is
 *	new enough (if an incremental is being done), it also checks
 *	if the file should be ignored because of the pattern match of
 *	it's name.
 *
 *	Returns:
 *
 *	NORMAL if file is rejected on above basis.
 *
 *      else filegrabber() return status.
 */

call_grabber(name, pathname, stats, function)
char	*name;
char	*pathname;
struct	stat	*stats;
int	function;
{
	/*
	 *	If we are doing an incremental, check that
	 *	the file is new enough. For a full backup,
	 *	Inc_date = 0.
	 */
	if (stats->st_mtime < Inc_date)
		return NORMAL;

	/*
	 *	If we are "-F" pattern ignoring,
	 *	check..
	 */
	if (Matchpatterns && nmatch(pathname, ig))
		return NORMAL;
	
	return(filegrabber(name, pathname, stats, function));
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : filegrabber()
 *	Purpose ............... : Reads files into shared memory. 
 *
 *	Description:
 *
 * 		Filegrabber() is the very non-re-entrant function,
 *		which is given a "name" of a file which it reads in to
 *		the packet transfer space of shared memory.
 *		
 *		Filegrabber() has three "functions":
 *
 * 		function:
 *
 * 		NORMAL 	Read in the file described by Pathname
 *		START	Initialise ourselves to start of first buffer
 *		FINISH	Finish off current buffer, and inform tapewriter
 *			that last buffer has been completed.
 *		RESTART	Restart from last checkpoint.
 *
 * 	Filegrabber keeps track of what buffer it is up to, and only
 * 	returns when it needs a new file. Filegrabber() ensures that
 *	a buffer is ready to read into when it returns.
 *
 *	Returns:
 *		0	-	if normal.
 *		1	-	if a restart is required.
 *		-1	-	if an abort is required.
 */
static
filegrabber(name, pathname, stats, function)
char	*name;
char	*pathname;
struct	stat	*stats;
int	function;
{
	char	*fname;
	int	spaceleft,
		file_section,
		read_ret,	
		headersize,
		current_fd = -1, /* Initialize with illegal file descriptor. */
		name_len;
	unsigned read_size;
	long	ftype,
		file_size,
		file_bytes_left;

	long	lseek();

	switch(function)
	{
	case NORMAL:	/* normal file entry */
		/*
		 *      Set the file type, if we have a special file,
		 *	then make sure the -x option has been used.
		 */
#ifdef ACLS
	        if(!Aclflag) {
		    if(stats->st_acl)
		        fprintf(stderr,"Optional acl entries for <%s> are not backed up.\n", pathname);
		}
#endif /* ACLS */
		ftype = stats->st_mode & S_IFMT;
		if (ftype != S_IFREG
		 && ftype != S_IFDIR
#ifdef SYMLINKS
		 && ftype != S_IFLNK
#endif SYMLINKS
		   )
		{
			if (!Dospecial)
			{
				(void)ftio_mesg(FM_NDX, pathname);
				return NORMAL;
			}

			if (!(ftype == S_IFNWK || ftype == S_IFCHR || 
			      ftype == S_IFBLK || ftype == S_IFIFO
			     )
			)
			{
				(void)ftio_mesg(FM_UNSPX, pathname);
				return NORMAL;
			}
		}

		/*
		 *	Don't ship the body of special files (except 
		 *	Network files) or zero length files or directory
		 *	files.
		 */
		if (
		   (ftype != S_IFREG
		 && ftype != S_IFNWK
#ifdef SYMLINKS
		 && ftype != S_IFLNK
#endif SYMLINKS
		   )
		   ||
		   stats->st_size == 0
		)
		{
			file_size = file_bytes_left = 0;

			/*
			 *	Don't ship contents of directory.
			 */
			if (ftype == S_IFDIR)
				stats->st_size = 0;
		}
		else
		{
			/*
			 *	Open the file to be backed up.
			 */
#ifdef SYMLINKS
			if ( !Symbolic_link && (current_fd = open(name, 0)) < 0)
#else
			if ((current_fd = open(name, 0)) < 0)
#endif SYMLINKS
			{
				/* could not open %s */
				(void)ftio_mesg(FM_NOPEN, pathname);	
				return NORMAL;
			}
			file_size = file_bytes_left = stats->st_size;
		}

		/*
		 *	Make the name that will be backed up.
		 *	Strip "./" from the front of a name,
		 *	or if it is an absolute pathname, conditionally
		 *	make it relative.
		 */
		fname = pathname;
		if (*fname == '.' && *(fname + 1) == '/')
			fname += 2;
		else if (Makerelative && *fname == '/')
			fname++;
		
		/*
		 *	If in verbose mode, list the file name
		 */
		if (Listfiles)
			puts(fname);

		/*
		 * 	If checkpointing, copy the name to the restart
		 *	file list.
		if (Checkpoint && !Filelist)
		{
			fputs(pathname, Restart_fp);
			fputc('\n', Restart_fp);  
		}
		 */

		/*
		 *	The first thing to do is write a header.
		 */
		file_section = HEADER_1;
		break;

	case RESTART: /* restart */
		cur_pkt = 0;
		cp = Packets[cur_pkt].block;
		ep = cp + Blocksize;
		(void)reset_sems();
		wait_packet(INPUT);
		
		/*
		 *	Restore the file stats.
		 */
		stats = &Packets[Nobuffers].stats;

		/*
		 * 	Restore	file_section.
		 */
		file_section = Packets[Nobuffers].file_section;

		/*
		 *      If this file has its contents shipped, re-open
		 *	it.
		 */
		ftype = stats->st_mode & S_IFMT;
		if (ftype == S_IFREG || ftype == S_IFDIR || ftype == S_IFNWK)
		{
			if ((current_fd = open(name, 0)) == -1)
			{
				/* could not open %s */
				(void)ftio_mesg(FM_NOPEN, pathname);	
				file_section = PAD;
			}
		}

		/*
		 *	Recreate name that gets shipped.
		 */
		fname = pathname;
		if (*fname == '.' && *(fname + 1) == '/')
			fname += 2;
		else if (Makerelative && *fname == '/')
			fname++;

		switch(file_section)
		{
			case HEADER_1:
			case FILENAME_1:
			case BODY_1:
				file_size = file_bytes_left 
					  = Packets[Nobuffers].file_size;
				break;

			case HEADER_2:
				name_len = strlen(fname);
				(void)writeheader(save_hdr, name_len, stats);
				spaceleft = Packets[Nobuffers].offset;
				break;

			case FILENAME_2:
				spaceleft = Packets[Nobuffers].offset;
				break;

			case BODY_2:
				/*
				 * 	Seek to the saved part in the file.
				 */
#ifdef SYMLINKS
				/*
				 * Symbolic links are going to cause
				 * problems here because we can't lseek(2)
				 * on the contents of a symbolic link.
				 * Not sure how to deal with this.
				 *
				 * For now, give warning message.
				 */
				if ( Symbolic_link )
				    ftio_mesg(FM_SYMMB, pathname);
				else
#endif SYMLINKS
				if (lseek(current_fd, 
					Packets[Nobuffers].offset, 0) == -1
				)
				{
					(void)ftio_mesg(FM_NSEEK, pathname);
					file_section = PAD;
				}
				file_size = Packets[Nobuffers].file_size;
				file_bytes_left = Packets[Nobuffers].file_size
						- Packets[Nobuffers].offset;
					
				break;

			case PAD:	
				file_size = Packets[Nobuffers].file_size;
				file_bytes_left = Packets[Nobuffers].file_size
						- Packets[Nobuffers].offset;
				break;
		}
		break;

	case START: /* initialise */
		cur_pkt = 0;
		cp = Packets[cur_pkt].block;
		ep = cp + Blocksize;
		wait_packet(INPUT);
		return NORMAL;

	case FINISH_2: /* finish off */
		file_bytes_left = 0;
		stats->st_size = 0;
		stats->st_mode = S_IFREG;

		/*
		 *	Use "TRAILER!!!" as the filename.
		 */
		fname = "TRAILER!!!";

		/*
		 *	The first thing to do is write a header.
		 */
		file_section = HEADER_1;
		break;

	case FINISH:	
		(void)filegrabber((char *)0, (char *)0, stats, FINISH_2);
		Packets[cur_pkt].status = PKT_FINISH;
		release_packet(OUTPUT);
		return  NORMAL;
	}


	headersize = (Htype)? CHARS: HDRSIZE;
	name_len = strlen(fname) + 1;

	while(1)
	{
		/*
		 *	We are at the start of a block trying to fill it. 
		 *	Checkpoint the current status.
		 */
		if (cp == Packets[cur_pkt].block && Checkpoint)
		{
			/*
			 * 	I wonder if we need to do this on every
			 *      occasion. How expensive are these things??
			 *	(memcpy() I mean).
			 */
			(void)memcpy(&Packets[cur_pkt].stats, (char *)stats,
				sizeof(struct stat)
			);

			Packets[cur_pkt].fname_offset = File_number;
			Packets[cur_pkt].file_size = file_size;
			Packets[cur_pkt].file_section = file_section;

			if (file_section == BODY_2 || file_section == PAD)
				Packets[cur_pkt].offset = 
					file_size - file_bytes_left;
			else
				Packets[cur_pkt].offset = spaceleft;
		}

		/*
		 *	Loop filling the buffer, whilst the current
		 *	position is less than the end position.
		 */
		while(cp < ep)
		{
			/*
			 * 	NOTE: a break out of this switch, will get a
			 *	new packet, when cp >= ep.
			 */
fs_switch:
			switch(file_section)
			{
			case HEADER_1:
				/*
				 *	Make sure the header starts on an
				 *	short boundary for binary headers.
				 */
				if (!Htype && ((int)cp & 1))
					*(cp++) = '\0';
				
				/*
				 *	Check to see we have enough room
				 *	left in this block. If we don't
				 *	fill the block with what we can,
				 *	then get a new block.
				 */
				spaceleft = ep - cp;
				if (spaceleft < headersize)
				{
					/*
					 * There is not enough space in this
					 * block to fit the header in.
					 */
					(void)writeheader(save_hdr, name_len, 
						stats
					);
					(void)memcpy(cp, save_hdr, spaceleft);
					file_section = HEADER_2;
					cp = ep;
					break;
				}

				/* 
				 * Plenty of room for the header
				 */
				cp += writeheader(cp, name_len, stats);
				file_section = FILENAME_1;
				/* FALL THROUGH */

			case FILENAME_1:
				/*
				 *	Check to see there is space left
				 *	for the filename, if there is not,
				 *	we must copy into the block what
				 *	we can, then get a new block.
				 */
				spaceleft = ep - cp;
				if (spaceleft < name_len)
				{
					(void)memcpy(cp, fname, spaceleft);
					file_section = FILENAME_2;
					cp = ep;
					break;
				}

				/*
				 *	Plenty of room for name.
				 */
				(void)memcpy(cp, fname, name_len);
				cp += name_len;
				file_section = BODY_1;
				/* FALL THROUGH */

			case BODY_1:
				if (!Htype && ((int)cp & 1))
					*(cp++) = '\0';
				/*
				 *	If the size of the current file
				 *	is zero, we don't have to ship
				 *	the file contents, only the header.
				 */
				if (!file_bytes_left)
					return NORMAL;

				file_section = BODY_2;

				/*
				 *	Once in the millenium, the above
				 *	cp++ will cause us to fill a block.
				 */
				if (cp >= ep)	
					break;

				/* FALL THROUGH */
			
			case BODY_2:
				/*
				 *	Read as much as is possible.
				 *	If the file has fewer bytes left
				 *	than in the block, then only
				 *	read that much, else fill the block.
				 */
				read_size = (file_bytes_left < (ep - cp))?
						file_bytes_left:
						(ep - cp);	

				/*
				 *	I wonder if we should PAD out after
				 *	a read failure??
				 *	We should! Else the restore will get
				 *	out of sync and lose the next file.
				 */

#ifdef SYMLINKS
				if ( Symbolic_link ) {
				    if ( (read_ret=readlink(name, cp, read_size)) == -1 ) {
					/* error reading %s */
					(void)ftio_mesg(FM_RDLINK, pathname);
					file_section = PAD;
					goto fs_switch;
				    }
				    cp[read_ret] = '\0'; /* Null terminate path. */
				} else
#endif SYMLINKS
				if ((read_ret = read(current_fd, cp, read_size))
				    == -1
				)
				{
					/* error reading %s */
					(void)ftio_mesg(FM_NREAD, pathname);
					file_section = PAD;
					close(current_fd);
					goto fs_switch;
				}

				file_bytes_left -= read_ret;	
				/*
				 *	Check to see read(2) did what was
				 *	expected.
				 */
				if (read_ret == (int)read_size)
				{
					if (file_bytes_left)
					{
						cp = ep;
						break;
					}
					
					/*
					 * Ok done with the file..
					 */
#ifdef SYMLINKS
					/*
					 * Only close "current_fd"
					 * if not symbolic link.
					 */

					if ( !Symbolic_link )
#endif SYMLINKS
					    close(current_fd);

#ifdef SYMLINKS
					if ( !Symbolic_link && Doacctime &&
#else
					if ( Doacctime &&
#endif SYMLINKS
					     set_time(name,
							stats->st_atime,
							stats->st_mtime
					     ) == -1
					)
						ftio_mesg(FM_NMODT, pathname);
					cp += read_ret;
					return NORMAL;
				}


				/*
				 *	If we have a short read the file
				 *	has shrunk since we stat(2)ed it.
				 *	Sorry we don't notice files that
				 *	grow.
				 */
				(void)ftio_mesg(FM_WSIZE, pathname);
#ifdef SYMLINKS
				/*
				 * Only close "current_fd"
				 * if not symbolic link.
				 */

				if ( !Symbolic_link )
#endif SYMLINKS
				    close(current_fd);

#ifdef SYMLINKS
				if ( !Symbolic_link && Doacctime &&
#else
				if ( Doacctime &&
#endif SYMLINKS
				     set_time(name,
						stats->st_atime,
						stats->st_mtime
				     ) == -1
				)
					ftio_mesg(FM_NMODT, pathname);
				cp += read_ret;
				file_section = PAD;
				goto fs_switch;

			case HEADER_2:
#ifdef	DEBUG
				if (Diagnostic)
				    printf("XXX: header across boundary!\n");
#endif	DEBUG
				/* 
				 *	Get next part of header into
				 *	a new block.
				 */
				(void)memcpy(cp, save_hdr + spaceleft, 
					headersize - spaceleft
				);
				cp += headersize - spaceleft;
				file_section = FILENAME_1;
				goto fs_switch;
					
			case FILENAME_2:
#ifdef	DEBUG
				if (Diagnostic)
				    printf("XXX: filename across boundary!\n");
#endif	DEBUG
				/* 
				 *	Get next part of filename into
				 *	the new block.
				 */
				(void)memcpy(cp, fname + spaceleft, 
					name_len - spaceleft
				);
				cp += name_len - spaceleft;
				file_section = BODY_1;
				goto fs_switch;

			case PAD:
				if (file_bytes_left < (ep - cp))
				{
					cp += file_bytes_left;
					return NORMAL;
				} else
					file_bytes_left -= (ep - cp);
				cp = ep;
				break;

			default:
				(void)ftio_mesg(FM_UNEX, "filegrabber()");
				return ABORT;
			}
		} /* while(packet isn't full) */

		Packets[cur_pkt].status = PKT_OK;
		release_packet(OUTPUT);
		cur_pkt = (cur_pkt < Nobuffers - 1)? cur_pkt + 1: 0;
		wait_packet(INPUT);

		/*
		 *	Check if we writer has sent RESTART message, etc.
		 */
		switch(Packets[cur_pkt].status)
		{
			case PKT_OK:
				break;

			case PKT_CHANGE:
				if (Checkpoint)
			      	      (void)memcpy(&Packets[Nobuffers], 
					       &Packets[cur_pkt],
					       sizeof(struct ftio_packet)
				      );
				break;

			case PKT_RESTART:	
				return RESTART;

			case PKT_ABORT:
				return ABORT;

			default:
				(void)ftio_mesg(FM_UNEX, "filegrabber()");
				return ABORT;
		}

		/*
		 *	Initialise pointers to the start of the
		 *	block, and the end of the packet.
		 */
		cp = Packets[cur_pkt].block;
		ep = cp + Blocksize;

	}/* while(bytes left to read from file) */
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : getfile()
 *	Purpose ............... : get a file name from file_fd. 
 *
 *	Description:
 *
 *	Getfile reads a file descriptor looking for file names. 
 *
 *	Returns:
 *		>=0	if a file was found.
 *		-1	if EOF was found.
 *
 *	History:
 *		920604	J. Lee	Modified the fgets call from the input stream
 *				to handle the possibility of a signal interr-
 *				upting the call.  If a interrupt does occur
 *				(EINTR signal) then determine if fgets needs
 *				to be re-driven (ignore interrupt), or handle
 *				as a EOF.
 *
 */

getfile(fp)
FILE	*fp;
{
	char	*p;
	char	rc;	/* Return code from fgets */

	/*
	 *	Loop reading files until:
	 *
	 *		1) we find a file that is stat(2)able
	 *		   in which case return(0);
	 *		2) we find EOF, return -1.
	 */
	while (1)
	{

		rc = NULL;
		while (rc == NULL)  /* Loop until a filename has been read */
		{
			rc = fgets(Pathname, MAXPATHLEN, fp);
			if ( rc == NULL )
			   if ( (errno == EINTR) && !(cld_flag))  
			   {
			     /* The FGETS call was interrupted and the signal */
			     /* trap handler has indicated that the interrupt */
			     /* was associated with the death of a child      */
			     /* process that is not the ftio co-process.  As  */
			     /* such, we will ignore this error message and   */
			     /* try to read from the input stream again.      */
			      errno = 0;	/* Reset Error Value. */
			   }
			   else
			   {
			     /* An unknown/unpredictable error condition has  */
			     /* occurred and will be treated as the same as   */
			     /* a EOF condition.   			      */
			      return(-1);
 			   }
		} /* end while (rc) 	*/

		/*
		 * 	Take the '\n' of the name.
		 */
		if (p = strchr(Pathname, '\n'))
			*p = '\0';

		/* 
		 * 	Attempt to stat(2) the file.
		 */
#ifdef SYMLINKS
		if(lstat(Pathname, &Stats) < 0)
#else
		if(stat(Pathname, &Stats) < 0) 
#endif SYMLINKS
			ftio_mesg(FM_NSTAT, Pathname);
		else
		{
		    (void) symlink_check(&Pathname, &Pathname, &Stats);
#ifdef DEBUG
if ( Symbolic_link ) fprintf(stderr, "ftio: symbolic link <%s>\n", Pathname);
#endif DEBUG

#if defined(DUX) || defined(DISKLESS)
#ifdef DEBUG
fprintf(stderr, "pre findpath: <%s>\n", Pathname);
#endif DEBUG
		    (void) findpath(Pathname);
#ifdef DEBUG
fprintf(stderr, "post findpath: <%s>\n", Pathname);
#endif DEBUG
#endif DUX || DISKLESS
		    return(0);
		}
	}
}


/*
 *-----------------------------------------------------------------------------
 *
 *      Title ................. : symlink_check()
 *      Purpose ............... : special handling for symbolic links
 *
 *	Description:
 *
 *      If we need to follow symbolic links (i.e. use the file the link
 *      points at instead of the link itself), do a stat(2) of the real
 *      file. Also, set flag if file is symbolic link and it must be
 *      stored as such.
 *
 *      NOTE: that as a side-effect of this routine it may modify
 *            the stat(2) buffer "statp", the path name "pathp" and
 *            the file name "filename".
 *
 *	Returns:
 *              1       if original path is symbolic link
 *              0       otherwise
 */

symlink_check(pathp,filenamep,statp)
    char **pathp;
    char **filenamep;
    struct stat *statp;
{
#ifdef SYMLINKS
    struct stat save;                   /* Save original stat(2) info. */
    static char namebuf[MAXPATHLEN];    /* Scratch pad. */
    int rv;                             /* Readlink(2) return value. */
    int exitval;                        /* Value used on return. */

    /*
     * Get the real file stats iff:
     *     1. the user wants to archive the real file
     *        instead of the symbolic link itself
     *        (i.e. the "h" option was used), and
     *     2. this file is a symbolic link.
     */

    save = *statp;  /* Save original stat(2) info. */

    if ( (statp->st_mode & S_IFMT) == S_IFLNK ) {
	exitval = 1;
	if ( Realfile ) {
	    if ( stat(*filenamep, statp) == -1 ) {
		(void)ftio_mesg(FM_NSTAT, *pathp);
		*statp = save;      /* Restore original stat(2) info. */
		Symbolic_link = 1;  /* Use the symbolic link anyway. */
		return exitval;
	    }

	    if ( (rv=readlink(*filenamep,namebuf,MAXPATHLEN)) == -1 ) {
		(void)ftio_mesg(FM_RDLINK, *filenamep);
		*statp = save;      /* Restore original stat(2) info. */
		Symbolic_link = 1;  /* Use the symbolic link anyway. */
		return exitval;
	    }

	    /*

	    if ( namebuf[0] == '/' ) {
		strncpy(*pathp,namebuf,rv);
		(*pathp)[rv] = '\0';
		*filenamep = *pathp;
	    } else {
		strncpy(*filenamep,namebuf,rv);
		(*filenamep)[rv] = '\0';
	    }

	     */

	}
    } else {
	exitval = 0;
    }

    if ( (statp->st_mode & S_IFMT) == S_IFLNK )
	Symbolic_link = 1;
    else
	Symbolic_link = 0;

    return exitval;
#else
    return 0;
#endif SYMLINKS
}



/*
 *-----------------------------------------------------------------------------
 *
 *      Title ................. : findpath()
 *      Purpose ............... : scan path for CDF components
 *
 *	Description:
 *
 *
 *      Given a path ( possibly an unexpanded cdf with ..,.,.+ & ..+ in it)
 *      return a fully expanded path, a bitstring which contains
 *      the permissions of each component, and the number of components.
 *
 *
 *      Returns: expanded path (returned through "path" argument)
 */

#if defined(DUX) || defined(DISKLESS)

static char fullpath[MAXPATHLEN];   /* See the "init_findpath" routine  */
static char *rest_of_path;          /* for more information.            */

findpath(path)
    char *path;
{
    int bitindex;
    int bitstring[MAXCOMPONENTS];
    static char tmp[MAXPATHLEN];
    short pass;
    char *p,*q,*r,*s,*lastslash;
    int end;
    char ts,save;
    struct stat t,dest;

    /*
     * Insure absolute path name.
     */

    if ( *path == '/' )
	strcpy(fullpath,path);
    else
	strcpy(rest_of_path,path);


    bitindex = end = 0;
    pass = -1;


#ifdef DEBUG
fprintf(stderr, "findpath: path <%s>\n", path);
#endif DEBUG

    /*
     * Remove extra slashes in pathname.
     */

    for (p=fullpath; *p; p++) {
	if ( *p == '/' && *(p+1) == '/' ) {
	    for (q=p+2; *q == '/'; q++)
		;
	    overlapcpy(p+1,q);
	}
    }

#ifdef DEBUG
fprintf(stderr, "findpath: fullpath <%s>\n", fullpath);
#endif DEBUG

RESET:
    bitindex=0;
    p = fullpath;
    lastslash = p;
    if (*p == '/') {
	lastslash = p;
	p++;
    }

    while ( (q=strchr(p,'/')) != (char *)NULL || *p ) {
	if ( *q == '\0' && *p != '\0' ) {
	    q=p;
	    while (*q)
		q++;
	    end =1;
	}

	/* get a component by putting a null */
	save = *q;
	*q = '\0';
	if (!strcmp(lastslash,"/..") || !strcmp(lastslash,"/..+")) {
	    if (lastslash == fullpath ) {
		strcpy(fullpath,"/");
		if (!end)
		  strcat(fullpath,q+1);
		p=q;
		continue;
	    } else
		*lastslash=0;

	    (void) getcdf(fullpath,tmp,MAXPATHLEN);
	    if(!strcmp(fullpath, tmp)) {
		*lastslash = '/';
		*q = save;
		lastslash = q;
		p = q+1;
		continue;
	    }

	    *lastslash='/';
#ifdef SYMLINKS
	    if ( lstat(fullpath,&dest) == -1 )
#else
	    if ( stat(fullpath,&dest) == -1 )
#endif SYMLINKS
		(void)ftio_mesg(FM_NSTAT, fullpath);
	    r = tmp;
	    if ( *r == '/' )
		r++;

	    while ( ((s=strchr(r,'/')) != (char*)NULL) || *r ) {
		if ( *s == '\0' && *r ) {
		    s=r;
		    while (*s++)
			;
		}
		ts = *s;
		*s = '\0';
#ifdef SYMLINKS
		if ( lstat(tmp,&t) == -1 )
#else
		if ( stat(tmp,&t) == -1 )
#endif SYMLINKS
		    (void)ftio_mesg(FM_NSTAT, tmp);

		if (t.st_ino == dest.st_ino &&
		    t.st_cnode == dest.st_cnode &&
		    t.st_dev == dest.st_dev ) {
		    if (!end) {
			strcat(tmp,"/");
			strcat(tmp,q+1);
		    } else
			p = q;
		   break;
		}

		*s =ts;
		if ( *s == '\0' )
		    break;
		r=s+1;
	    }

	    strncpy(fullpath,tmp,MAXPATHLEN);
	    p=fullpath;
	    if (*p == '/') {
		lastslash = p;
		p++;
	    }
	    continue;
	}

	/* ordinary file */
	/* if a cdf and there is a plus at the end, take it out
	  else just sit bitstring */
	switch (pass) {
	    case -1:             /* first pass , just increment ptrs */
		*q=save;
		lastslash = q;
		break;

	    case 0:              /* second pass, stat */
#ifdef SYMLINKS
		if ( lstat(fullpath,&t) == -1 )
#else
		if ( stat(fullpath,&t) == -1 )
#endif SYMLINKS
		    (void)ftio_mesg(FM_NSTAT, fullpath);
		bitstring[bitindex++]=t.st_mode;
		*q=save;
		lastslash = q;
		break;

	    case 1:                 /* Third pass, change '+'
				       to "+/"  */
		t.st_mode = bitstring[bitindex];
		*q=save;
		lastslash = q;
		if ( ( (t.st_mode & S_IFMT) == S_IFDIR ) &&
		     (  t.st_mode & S_ISUID )
		   ) {
		    if ( ( q > fullpath ) && (*(q-1) == '+') ) {
			strcpy(tmp,q);
			strcpy(q,"/");
			*(q+1) = '\0';
			strcat(fullpath,tmp);
			q++;
		    }
		}
		bitindex++;
		lastslash = q;
		break;

	    default:
		break;
	}
	if ( *q == '\0' )
	    break;
	p=q+1;

    }

#ifdef DEBUG
fprintf(stderr, "findpath: fullpath <%s> (pass %d)\n", fullpath, pass);
fprintf(stderr, "working directory: %s\n\n", getcwd((char *)0, 50));
#endif DEBUG

    if ( ++pass < 2 )
	goto RESET;


    /* Copy new pathname back into "path". */

    if ( *path == '/' )
	strcpy(path,fullpath);
    else
	strcpy(path,rest_of_path);

    return;
}


init_findpath()
{

    /*
     * Set up buffer and pointer to insure absolute path
     * for findpath() routine.
     *
     *
     * The "fullpath" string contains the following:
     *
     *      <cwd>/<rest_of_path>
     *
     * where <cwd> is the current working directory prefix
     * and <rest_of_path> is the actual path name that we're
     * processing. Together these two sections form the
     * absolute path name of a file.
     *
     * The "rest_of_path" pointer points to the <rest_of_path>
     * portion of "fullpath".
     */

    strcpy(fullpath, Home);
    rest_of_path = &fullpath[strlen(fullpath)];


    /*
     * Insure termination of "fullpath" with a slash.
     */

    if ( fullpath[strlen(fullpath)-1] != '/' ) {
	strcpy(rest_of_path,"/");
	rest_of_path++;
    }
}



#endif DUX || DISKLESS



overlapcpy(s1,s2)
    char *s1;
    char *s2;
{

    /*
     * Copy overlapping areas.
     * Don't forget '\0' character at end.
     */

    for (; *s2; s1++,s2++)
	*s1 = *s2;
    *s1 = '\0';
}
