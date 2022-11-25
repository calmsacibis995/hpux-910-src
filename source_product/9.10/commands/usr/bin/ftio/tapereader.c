/* HPUX_ID: @(#) $Revision: 72.1 $  */
/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : tapereader.c 
 *	Purpose ............... : Read blocks from tape. 
 *	Author ................ : David Williams, ASO. 
 *
 *	Description:
 *
 *	
 *	Contents:
 *
 *-----------------------------------------------------------------------------
 */

#include	"ftio.h"
#include	"define.h"

tapereader(tape_fd)
int	tape_fd;
{
	int     cur_pkt, prev_pkt;
	int	noblocks = 0;
	int	readret;
	int	tapeno = Tape_no;
#ifdef PKT_DEBUG
	int     i;
#endif PKT_DEBUG

#ifdef hp9000s500
	extern 	int 	errinfo;
#else
	extern	int	errno;
#endif

	/*
	 * 	Define the buffer we want to fill first (buffer #0)
	 */
	cur_pkt = 0;
	prev_pkt = 0;

	/*
	 * 	Loop reading blocks forever
	 */
	while(1)
	{
		/*
		 * 	Wait on the next buffer to have been emptied
		 * 	by the filewriter process.
		 */
#ifdef PKT_DEBUG
printf("ftio: tapereader: waiting to fill next packet %d (INPUT=%d).\n", cur_pkt, get_val(INPUT));
fflush(stdout);
#endif PKT_DEBUG
		wait_packet(INPUT);	
#ifdef PKT_DEBUG
printf("ftio: tapereader: done waiting to fill next packet %d (INPUT=%d).\n", cur_pkt, get_val(INPUT));
fflush(stdout);
#endif PKT_DEBUG
		

		/*
		 *      Check previous packet to see if the filewriter
		 *      process indicates termination. I.e., does the
		 *      status field indicate we're done?
		 */

#ifdef PKT_DEBUG
for (i=0; i < Nobuffers; i++)
    if ( Packets[i].status == PKT_FINISH )
	printf("ftio: tapereader: found PKT_FINISH in packet %d (cur_pkt=%d,prev_pkt=%d).\n", i, cur_pkt, prev_pkt);
fflush(stdout);
#endif PKT_DEBUG

		if ( Packets[prev_pkt].status == PKT_FINISH )
		    return noblocks;


		/*
		 * 	Read in big chunks from the input device
		 */
		while ( (readret = rmt_read(tape_fd, Packets[cur_pkt].block,
				     Blocksize
				   )
			) <= 0
		)
		{

			/*
			 *	Test for this is last tape.
			 */
			switch(test_eot(tape_fd, readret))
			{
			case FINISH:
				Packets[cur_pkt].status = PKT_FINISH;
				release_packet(OUTPUT);
				return noblocks;
			
			case ABORT:
				Packets[cur_pkt].status = PKT_ABORT;
				release_packet(OUTPUT);
				ftio_mesg(FM_NREAD, Output_dev_name);
				return noblocks;

			default:
				break;
			}

			/* 
			 * 	Stop the performance timer, while we
			 * 	change tapes.
			 *	Print current performance data.
			 *	Change the tape.
			 *	Then start timer again.
			 */
			printperformance(0, 1);

			if (Performance > 1)
				printperformance(noblocks, 3);

			if ((tape_fd = chgreel(tape_fd,INPUT, tapeno+1)) == -1)
			{
				Packets[cur_pkt].block_size = 0;
				Packets[cur_pkt].status = PKT_FINISH;
				return noblocks;
			}

			/*
			 *	Update the tape number, because we may have
			 *	skipped.
			 */
			tapeno = Tape_no;

			/*
			 *	Start timer again.
			 */
			printperformance(0, 2);
		}
#ifdef	DEBUG
		if (Diagnostic)
			if (readret < Blocksize)
				printf("XXXX %s: short block, size is: %d\n", 
				Myname, readret);
#endif	DEBUG

		Packets[cur_pkt].block_size = readret;

		/*
		 * ok finished with that buffer
		 */
#ifdef PKT_DEBUG
printf("ftio: tapereader: releasing packet %d (OUTPUT=%d).\n", cur_pkt, get_val(OUTPUT));
fflush(stdout);
#endif PKT_DEBUG
		release_packet(OUTPUT);
		
		noblocks++;

		/* 
		 * update the buffer count
		 */
		prev_pkt = cur_pkt;
		cur_pkt = (cur_pkt < (Nobuffers-1))? (cur_pkt + 1): 0;
	}
}
