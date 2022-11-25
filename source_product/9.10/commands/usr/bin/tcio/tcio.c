/* @(#) $Revision: 72.1 $ */     
#ifdef	hp9000s800
#endif	hp9000s800
/*
 *  tcio   
 */

/*
 *   Tcio handles buffering to and from linus type 
 *   cartridges.  All machine specific operations 
 *   are contained in the module "tpmdep.c". There
 *   is also a utility mode to do integral media
 *   copies, release of tape cartridge, marking 
 *   the tape with a file mark, and verifying the tape.
 *   Current limitations:
 *	Media copy should be allowed between tape/disc 
 *	(on separate controllers) and disc/disc.
 *	Check digit calculation should be more
 *	robust.
 */

#include "tcio.h"
#include "tcio.def"

main(argc,argv)
int  argc;
char *argv[];
{

	char	*malloc();
	int     Option;
	int	c;
	int	block_number = -1;
	int	filestoskip = 0;
	int	request;
	int 	partial;
	register char *cp;
	int	stdout_ok;
	

	parse_option (&Option, &block_number, &filestoskip, argv, argc);

	if ( (fildes=open(fname,( Option == IN ) ? O_RDONLY : O_RDWR)) < 0 ) {
	    int save;
	    save = errno;
	    err("tcio(1004): cannot access tape %s; errno %d\n", fname, errno);
	    exit(save);
	}

	if (Merlinflag)  {  /* In Merlin-mode, we must first load cartridge */
		if (Merlinflag_n)	{
			if (num_cart < 1 || num_cart > 8 ||
				start_cart+num_cart > 9 ) {
			        fprintf(stderr,"tcio(1035): invalid limit number of cartridge.  The maximum number is %d\n.", 9 - start_cart);
			        exit(USEERR);
			}
		}
		else num_cart = 9 - start_cart;

		/* need to save original values */
		num_cart_orig = num_cart;
		start_cart_orig = start_cart;

		load_cart(fildes, start_cart++);
	}

	/* set desired tape options */
	set_options(fildes);

	/* calculate tape length in blocks */
	tape_length = bytes_per_medium(fildes) / BUFSIZE;

	/* allocate buffer space */
	if (Option != UTILITY) {
		if (!bufflag)
			maxindex = DEFINDEX;
		if ((buf = malloc(maxindex)) == NULL) {
			err("tcio(1005): Unable to allocate buffer; size %d\n", maxindex);
			rel_exit(BUFERR);
		}
	}

	switch (Option)
		{
		case OUT :
			/* put tape mark at front of tape to prevent */
			/* image restore using switches */
			if (!Systemflag)
				write_tape_mark (fildes, 0, EXIT);
			/* skip past required number of files */
			while (filestoskip-- > 0) {
				while (bread() >= 0)
					/* keep reading */;
				lseek(fildes, BUFSIZE, 1);
			}
			start_blk = lseek(fildes, 0, 1) / BUFSIZE;
			total_blk = num_blk = 0;
			/* read input and buffer to tape */
			do {
				bufindex = 0;
				request = maxindex;
				while (request)
					{
					if ((c=read(0,buf+bufindex,request)) > 0)
						{
						request -= c;
						bufindex += c;
						}
					else
						break;
					}

				/* fill any partial block with nulls */
				if (partial = bufindex % BUFSIZE) {
					cp = buf + bufindex;
					bufindex += BUFSIZE - partial;
					while (cp < buf+bufindex)
						*cp++ = 0;
				}

				/* check digit calculation */
				if (checkflag)
					for (cp = buf + bufindex; cp > buf; )
						checksum += *--cp;

				bwrite(bufindex);
			} while (c > 0);
			errv("tcio(1006): 1024 byte blocks written: %d\n",
							total_blk + num_blk);
			if (checkflag) 
				fprintf (stderr, "tcio(1007): Checksum value: 0x%x\n",
					checksum);
			if (eodmarkflag) {
				if (start_blk + num_blk < tape_length)
					write_tape_mark(fildes, start_blk + num_blk, EXIT);
			} else if (!Systemflag)
				write_tape_mark(fildes, tape_length-1, EXIT);
			
			if (verifyflag) verify_tape(fildes, start_blk, num_blk);
			if (releaseflag) unload_tape (fildes);
			break;
		case IN :
			signal(SIGPIPE, SIG_IGN);
			/* Skip the tape mark at the beginning of the tape */
			if (!Systemflag)
			  if (lseek (fildes, BUFSIZE, 0) == -1)
				{
				err("tcio(1008): cannot seek past EOF in first record; ");
				err("errno: %d file: %s\n", errno, fname);
				rel_exit(errno);
				}
			/* skip past required number of files */
			while (filestoskip-- > 0) {
				while (bread() >= 0)
					/* keep reading */;
				lseek(fildes, BUFSIZE, 1);
			}
			start_blk = lseek(fildes, 0, 1) / BUFSIZE;
			total_blk = num_blk = 0;
			/* Unbuffer the tape, put data to stdout */
			stdout_ok = 1;
			while (stdout_ok && (request = bread()) >= 0) {
				bufindex = 0;
				while (request) {
					if ((c=write(1,buf+bufindex,request)) > 0) {
						request -= c;
						bufindex += c;
					} else {
						stdout_ok = 0;
						break;
					}
				}
				/* check digit calculation */
				if (checkflag)
					for (cp = buf + bufindex; cp > buf; )
						checksum += *--cp;
			}
			errv( "tcio(1009): 1024 byte blocks read: %d\n",
							total_blk + num_blk);
			if (checkflag) 
				fprintf (stderr, "tcio(1010): Checksum value: 0x%x\n",
				checksum);
			if (releaseflag) unload_tape (fildes);
			break;
		case UTILITY:
			if (markflag)
				write_tape_mark (fildes, block_number, EXIT);
			if (verifyflag)
				/* Number of blocks specified? */
				if (block_number >= 0)
					verify_tape(fildes, 0, block_number);
				else
					/* if not verify whole tape */
					verify_tape(fildes, 0, tape_length);
			if (releaseflag) unload_tape(fildes);
			break;
		}
	close(fildes);
}

/*********** opentty ************
 * open the tty that initiated 
 * the program, if unsuccessful
 * talk directly to the console
 */
opentty(prompt,retstr)
char *prompt, *retstr;
{
	FILE *devtty;

	devtty = fopen(Alt_tty ? Tty : "/dev/tty", "r+");
	if (devtty == NULL )
		{
		err("tcio(1011): EOT and no input device available\n");
		rel_exit(NOINPERR);
		}
	fprintf(devtty, prompt);
	fflush(devtty);
	if (fgets(retstr, 20, devtty) == NULL)
		err("tcio(1012): error in read from console %d\n",
			ferror(devtty));
	retstr[strlen(retstr) - 1] = '\0';
	fclose(devtty);
}


/*********** samprompter ************
 * for sam, want prompt and reply
 * to go to and come from the named
 * pipe /tmp/sampipe which SAM uses. 
 */
samprompter(prompt,retstr)
char *prompt, *retstr;
{
	FILE *sampipe;
	sampipe = fopen("/tmp/sampipe", "r+");
	if (sampipe == NULL )
		{
		err("tcio(1034): couldn't open sampipe\n");
		rel_exit(NOINPERR);
		}
	fprintf(stderr, prompt);
	if (fgets(retstr, 20, sampipe) == NULL)
		err("tcio(1032): error in read from sampipe %d\n",
  		     ferror(sampipe));
	retstr[strlen(retstr) - 1] = '\0';
        fclose(sampipe);
}
