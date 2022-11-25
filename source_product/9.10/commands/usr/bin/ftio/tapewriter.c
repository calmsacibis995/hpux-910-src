/* HPUX_ID: @(#) $Revision: 72.1 $  */
/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : tapewriter.c 
 *	Purpose ............... : Routines to write to tape. 
 *	Author ................ : David Williams. 
 *
 *	Description:
 *
 *	Contents:
 *
 *-----------------------------------------------------------------------------
 */

#include	"ftio.h"
#include	"define.h"

tapewriter(tape_fd)
int	tape_fd;
{
	/* 
	 *	Good_blocks is a count of the blocks that have been 
	 *	written to the tape without the tape falling over.
	 *	So no_blocks is always up to date, good_blocks only
	 *	gets updated as each tape is successfully completed.
	 *	This allows a fall back situation for the no of blocks 
	 *	transferred data.
	 */
	int	no_blocks = 0;
	int	good_blocks = 0;	

	int	writeret;
	int	tapeno = 1;
	int	cur_pkt;

	int	last_block = 0; 
	int	tape_change = 0; 

	/*
	 * 	I hate to use a goto, but this is one of those places
	 *	where they are just great.
	 *
	 *	If we come through here we are starting or we are
	 *	recovering from a bad tape. The next call sets up
	 * 	the semaphore set, so that we have no packets available
	 *	and must wait on the other process to feed us some.
	 */
restart:
	cur_pkt = 0;

	/*
	 *	Loop until we have good reason to do otherwise.
	 */
	while(1)
	{
		/*
		 * 	Wait on the next packet to have been filled
		 * 	by the loader process.
		 */
		wait_packet(OUTPUT);
		
		/*
		 * 	Was that the last packet??
		 */
		if (Packets[cur_pkt].status == PKT_FINISH)
			last_block++;
		
		/*
		 * 	Write out big chunks to the output device
		 */
		while ((writeret = rmt_write(tape_fd, Packets[cur_pkt].block, 
			Blocksize))
			 != Blocksize
		)
		{
			tape_change++;

			/*
			 * 	Check error code to determine
			 *	what we should do..
			 */
			switch(test_write_err(tape_fd))
			{
			case NEXT:
				(void)printperformance(0, 1);
next:
				Packets[cur_pkt].status = PKT_CHANGE;
				tapeno++;
				if (Filelist)
					shuffle_filelist(tapeno, 
						Packets[cur_pkt].fname_offset
					);
				tape_fd = chgreel(tape_fd, OUTPUT, tapeno);
				good_blocks = no_blocks;
				break;
			
			case RESTART:	/* Restart tape if bad. */
				(void)printperformance(0, 1);
				(void)close(tape_fd);
				if (Filelist)
				{
					no_blocks = good_blocks;
					if (ftio_mesg(FM_BADT))
					{
						/*
						 *	Change made here
						 *	so that tape change
						 *	dialog is carried
					 	 * 	out before the
						 *	filewriter starts
						 *	writing file names
						 *	to the screen..28/4/1987
						 */
						tape_fd = chgreel(tape_fd, 
								  OUTPUT, 
								  tapeno
							  );	
						(void)printperformance(0, 2);
						Packets[cur_pkt].status 
							= PKT_RESTART;
						release_packet(INPUT);
						goto restart;
					}
				}
				else
				{
					if (ftio_mesg(FM_BADTNOL))
						goto next;
				}
				/* else */
				/* FALL THROUGH */

			case END:		/* huh?? */
			case ABORT:
			default:	/* write fail can't cope */
				Packets[cur_pkt].status = PKT_ABORT;
				release_packet(INPUT);
				ftio_mesg(FM_NWRIT, Output_dev_name);
				return no_blocks;
			}

			/*	
			 *	if -pp given, print performance data now. 
			 */
			(void)printperformance(0, 2);
			if (Performance > 1)
				(void)printperformance(no_blocks, 3);
			
			/*
			 * 	This is in here for personal interest
			 *	and because ftio < X.24 would break if this
			 *	ever got printed.
			 */
			if (writeret > 0)
			      printf("WARNING SHORT WRITE BLOCK!! please(!) contact your local sales office!!!\n");
		}

		/*
		 *	Did we have a tape change??
		 */
		if (tape_change)
		{
			Packets[cur_pkt].status = PKT_CHANGE;
			tape_change = 0;
		}
		else
		{
			Packets[cur_pkt].status = PKT_OK;
			/*
			 *	The first one must also be treated as 
			 *	a tape change - we must checkpoint it!
			 */
			if (!no_blocks)
				Packets[cur_pkt].status = PKT_CHANGE;
		}
		
		/*
		 *	Another one bites the dust..
		 */
		no_blocks++;
			
		release_packet(INPUT);

		/*
		 *	Finished??
		 */
		if (last_block)
			break;

		/* 
		 * 	Update the packet count
		 */
		cur_pkt = (cur_pkt < Nobuffers - 1)? cur_pkt + 1: 0;
	}

	return no_blocks;
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : shuffle_filelist()
 *	Purpose ............... : Update tape file list. 
 *
 *	Description:
 *
 *	Update Filelist, so that files still to be backed up have 
 *	the tape number they are on incremented.
 *
 *	Returns:
 *
 *	-1 if fail. 
 *
 */

shuffle_filelist(tape_no, line_no)
int	tape_no;
off_t	line_no;
{
	FILE	*fpa,
		*fpb;
	int	i, n;
	char	*p;

	Pathname_cpy(Filelist, strlen(Filelist));
	Pathname_cat(".t", 2);

	unlink(Pathname);       

	if (link(Filelist, Pathname) == -1)
		return -1;
	
	if (unlink(Filelist) == -1)
		return -1;

	if ((fpb = fopen(Filelist, "w")) == NULL)
		return -1;

	if ((fpa = fopen(Pathname, "r")) == NULL)
		return -1;
	
	/*
	 *	Loop and read all the backed up lines.
	 */
	for (i = 0; i <= line_no; i++)
	{
		/*
		 *	This is a kludge - it should use the clever
		 *	stuff to handle super long names.
		 *
		 *	Boy was it ever a kludge - I must have been asleep
		 *	when I wrote this. fscanf can't handle spaces, CR,
		 *	etc in a line!! What a dummy.
		 */
		if (fgets(Pathname, MAXPATHLEN, fpa) == NULL)
		{
			fprintf(stderr, 
				"not enough lines in tape directory list\n"
			);
			(void)fclose(fpa);
			(void)fclose(fpb);
			return -1;
		}

		/*
		 *	Check the tape number.
		 */
		n = atoi(Pathname);

		/*
		 *	Skip over the tape number.
		 */
		for (p = Pathname; *p && *p++ != ' ';);

		fprintf(fpb, "%d %s", n, p);
	}

	/*
	 *	Now copy on the files we wish to backup.
	 */
	while (fgets(Pathname, MAXPATHLEN, fpa) != NULL)
	{
		/*
		 *	Skip over the tape number.
		 */
		for (p = Pathname; *p && *p++ != ' ';);

		fprintf(fpb, "%d %s", tape_no, p);
	}
	
	(void)fclose(fpa);
	(void)fclose(fpb);

	Pathname_cpy(Filelist, strlen(Filelist));
	Pathname_cat(".t", 2);

	(void)unlink(Pathname);       

	return 0;
}

