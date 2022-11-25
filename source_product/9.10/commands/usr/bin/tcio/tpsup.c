/* @(#) $Revision: 64.2 $ */    
/*
 *  tcio utilities
 */
#include "tcio.h"
char  *prompt = "tcio(1024): To continue, type new device name when ready, return implies same device\n";
extern int num_cart_orig, start_cart_orig;
/* 
 *    read a buffer from the tape cartridge
 *    if the amount of data is less than a full
 *    buffer, then see if another tape is available
 *    in the tape set( no tape mark in last record)
 *    if another tape is available change reels.
 */
bread()
{
	char *p;
	int c;
	int	bytes_remaining;
	int	request;
	char	lbuf[BUFSIZE];
	
	if (fildes < 0) return (EOF);
	/* reset error flag so previous results will not affect outcome */
	errno = 0;
	p = buf;
	/* read only what is on the tape */
	bytes_remaining = (tape_length - start_blk - num_blk) * BUFSIZE;
	request = (maxindex <= bytes_remaining) ? maxindex : bytes_remaining;
	if (!Systemflag && request == bytes_remaining)
		request -= BUFSIZE;
	if((c = read(fildes, p, request)) < 0)
		{
		err("tcio(1025): errno: %d ", errno);
		errv("at physical block: %d, ", start_blk + num_blk);
		err("Can't read input tape");
		errv("; request %d",request);
		err ("\n");
		rel_exit(errno);
		}
	num_blk += (c/BUFSIZE);

	bytes_remaining = (tape_length - start_blk - num_blk) * BUFSIZE;
	if (!bytes_remaining || !Systemflag && bytes_remaining <= BUFSIZE) {
		if (!Systemflag) {
			errno = 0;
			if (read(fildes, lbuf, BUFSIZE) <= 0)
				return (EOF);
		}
		if (Merlinflag)
			Merlin_chgcart(IN, fildes);
		else fildes = chgreel (IN, fildes);
		return(c);	/* allow return of 0 bytes */
	}
	return(c ? c : EOF);	/* 0 bytes here means imbedded file mark */
}


/*
 *     write a buffer to the tape cartridge
 *     if less than the requested data is written
 *     because an end of tape would be encountered
 *     change reels.
 */
bwrite(needed)
int	needed;
{
	char *p;
	int c;
	int	bytes_remaining;
	int	request;

	p = buf;
	/* reset global error flags */
	errno = 0;
	do {
		/* don't overwrite end of tape */
		bytes_remaining = (tape_length - start_blk - num_blk) * BUFSIZE;
		request = (needed <= bytes_remaining) ? needed : bytes_remaining;
		if (!Systemflag && request == bytes_remaining)
			request -= BUFSIZE;
		if((c = write(fildes, p, request)) < request)
			{
			err("tcio(1026): errno: %d ", errno);
			errv("at physical block: %d, ", start_blk + num_blk);
			err("Can't write output tape");
			errv("; request %d",request);
			err("\n");
			rel_exit(errno);
			}
		num_blk += (c/BUFSIZE);
		needed -= c;
		p += c;

		bytes_remaining = (tape_length - start_blk - num_blk) * BUFSIZE;
		if (needed && (!bytes_remaining || !Systemflag && bytes_remaining <= BUFSIZE)) {
			if (!Systemflag) {
				errv("end of tape nb:%d, tl:%d\n",num_blk,tape_length);
				errno = 0;
				write(fildes, buf, BUFSIZE);
			}
			if (Merlinflag)
				Merlin_chgcart(OUT, fildes);
			else fildes = chgreel (OUT, fildes);
		}
	} while (needed > 0);
	return (c);
}


/*
 *      change tape cartridge
 *      verify and unload if necessary.  Open
 *      the tty or console that initiated
 *      tcio, and get the continuation device.
 *	Write or skip the tape mark at the 
 *	front of the tape which ever is appropriate.
*/
chgreel(x, fd_old)
{
	register fd_new;
	char str[22], retry;

	if (errno)
		{
		err("tcio(1031): errno: %d", errno);
		errv("at physical block: %d, ", start_blk + num_blk);
		err("Can't %s\n", x? "write output": "read input");
		if ((verifyflag) && (x==OUT)) 
			verify_tape(fd_old, start_blk, num_blk);
		rel_exit(errno);
		}
	else
		{
		/* only verify tapes that have been written */
		if ((verifyflag) && (x==OUT)) 
			verify_tape(fd_old, start_blk, num_blk);
Again:    /*
	   * Return to this point when unwritable tape is inserted into
	   * the tape drive.
	   */
		unload_tape (fd_old);
		}
	close(fd_old);
	fd_new = -1;
	total_blk += num_blk;
	num_blk = 0;
	retry = 0; /* Allow user to insert cartridge and not
		    * wait for preload to complete
		    */
	while (fd_new < 0)
		{
		
                if (!Xflag)
                   opentty(prompt,str);
                else
                   samprompter(prompt,str);
		if(!*str)
		    strncpy(str,fname,21);
		else
		    strncpy(fname,str,21);
		if((fd_new = open(str, x? 1: 0)) < 0) 
			{
			err("tcio(1027): That didn't work\n");
			continue;
			}
		/* set desired tape options */
		set_options(fd_new);
		/* find stats on new tape and prepare to continue */
		if ((tape_length = bytes_per_medium(fd_new)/BUFSIZE) == 0) {
			err("tcio(1028): Is Cartridge properly loaded?\n");
			close(fd_new);
			fd_new = -1;
			}
		}
	if (!Systemflag)
		{
		switch (x)
			{
			case OUT:
				if (write_tape_mark(fd_new,0,IGNORE_ERR) < 0) {
				    errv ("tcio(1036): New tape is unwritable tape.\n");
				    goto Again;
				}
				break;
			case IN:
				lseek (fd_new,BUFSIZE,0);
				break;
			}
		}
	start_blk = lseek(fd_new, 0, 1) / BUFSIZE;
	return (fd_new);
}

/*
 *      Merlin Mode: automatic change tape cartridge
 *      Verify and unload if necessary.  
 *	Write or skip the tape mark at the 
 *	front of the tape which ever is appropriate.
*/
Merlin_chgcart(oper, fildes)
{
	int n;
	char str[10];

	if (errno)
		{
		err("tcio(1029): errno: %d ", errno);
		errv("at physical block: %d, ", start_blk + num_blk);
		err("Can't %s\n", oper? "write output": "read input");
		if ((verifyflag) && (oper==OUT)) 
			verify_tape(fildes, start_blk, num_blk);
		rel_exit(errno);
		}
	/* only verify tapes that have been written */
	if ((verifyflag) && (oper==OUT)) 
		verify_tape(fildes, start_blk, num_blk);
	unload_tape (fildes);
	total_blk += num_blk;
	num_blk = 0;
	/* num_cart_orig, start_cart_orig  */
	if (--num_cart <= 0)	{ /* Prompt for additional media */
                if (!Xflag)
		   opentty("tcio(1030): Autochanger tapes filled. (Return implies original values.)\nStart Cartridge: ",str);
                else
		   samprompter("tcio(1030): Autochanger tapes filled. (Return implies original values.)\nStart Cartridge: ",str);

		n = 0;
		if (*str) n=atoi(str);
		if (n >= 1 && n <= 8) start_cart = n;
		else start_cart = start_cart_orig;

		if (!Xflag)
                  opentty("tcio(1033): Limit for number of cartridges: ",str);
                else
                  samprompter("tcio(1033): Limit for number of cartridges: ",str);
		n = 0;
		if(*str) n=atoi(str);
		if (n >= 1 && n <= 8 && n+start_cart <= 9) 
			num_cart=n;
		else num_cart = num_cart_orig;
	}
	load_cart(fildes, start_cart++);
	/* set desired tape options */
	set_options(fildes);
	/* find stats on next tape and prepare to continue */
	tape_length = bytes_per_medium(fildes)/BUFSIZE;
	if (!Systemflag)
		{
		switch (oper)
			{
			case OUT:
				write_tape_mark (fildes,0,EXIT);
				break;
			case IN:
				lseek (fildes,BUFSIZE,0);
				break;
			}
		}
	start_blk = lseek(fildes, 0, 1) / BUFSIZE;
}
