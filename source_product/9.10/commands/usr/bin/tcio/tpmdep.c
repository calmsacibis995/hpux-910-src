/* @(#) $Revision: 64.1 $ */   
/*
 *  tape support
 */

#include "tcio.h"

#ifdef	DEBUG
extern int Set_tape_size;
#endif	DEBUG

set_options(fildes)
int fildes;
{
	char option_byte = SO_SKIP_SPARING	|
			   SO_AUTO_SPARING	|
			   SO_IMMEDIATE_REPORT	;

	if (ioctl(fildes, CIOC_SET_OPTIONS, &option_byte) < 0) 
		{
		err("tcio(1013): cannot set tape options %s; errno %d\n",fname ,errno);
		exit(errno);
		}
}

bytes_per_medium(fildes)
int fildes;
{
	struct describe_type describe_bytes;
	int numblks, retry=1;

RETRY:
	if (ioctl(fildes, CIOC_DESCRIBE, &describe_bytes) < 0) 
		{
		if (retry)	{
			retry=0; /* Allow one retry after 10 secs */
			err("tcio(1014): retrying tape stats%s; errno %d\n");
			sleep(10);
			goto RETRY;
			}
		err("tcio(1015): cannot stat tape %s; errno %d\n",fname ,errno);
		exit(errno);
		}
	numblks =
#ifdef hp9000s800
		describe_bytes.volume_tag.volume.maxsvadd_lfb+1;
#else not hp9000s800
		describe_bytes.volume_tag.volume.maxsvadd.lfb+1;
#endif hp9000s800
	/* CS80 Describe will return 0 if no cartridge loaded */
	if (numblks-1==0)
		return(0);
#ifdef	DEBUG
	if (Set_tape_size)
		numblks = Set_tape_size;
#endif	DEBUG
	errv("tape size: %d blocks\n", numblks);
	return numblks * describe_bytes.unit_tag.unit.nbpb;
}

unload_tape(fildes)
int fildes;
{
	errv ("tcio(1016): Unloading tape %s; please wait ...\n",fname);
	if (ioctl(fildes, CIOC_UNLOAD, NULL) < 0) 
		{
		err("tcio(1017): cannot unload tape %s; errno %d\n",fname ,errno);
		exit(errno);
		}
}


verify_tape (fildes, start_block, number_blocks)
int	fildes;
unsigned	start_block;
unsigned	number_blocks;
{
	struct verify_parms verify_parms;

	verify_parms.start = start_block * BUFSIZE;
	verify_parms.length = number_blocks * BUFSIZE;
	errv ("tcio(1018): verify tape ");
	errv(" %d blocks; start block %d", number_blocks, start_block);
	errv (" \n");
	if (ioctl(fildes, CIOC_VERIFY, &verify_parms) < 0) 
		{
		err("tcio(1019): cannot verify tape %s; errno %d\n", fname, errno);
		exit(errno);
		}
}

int
write_tape_mark (fildes, block_number, err_operation)
int		fildes, err_operation;
unsigned	block_number;
{
	struct mark_parms mark_parms;

	mark_parms.start = block_number * BUFSIZE;
	if (ioctl(fildes, CIOC_MARK, &mark_parms) < 0 )
	    if (err_operation) {
		return(-1);
	    } else {
		err("tcio(1020): cannot write file mark tape %s; errno %d\n",
		       fname, errno);
		exit(errno);
	    }
	if (lseek (fildes, BUFSIZE ,1) == -1)
		{
		err("tcio(1021): cannot seek past file mark on tape; ");
		err("errno: %d tape: %s\n", errno, fname);
		exit(errno);
		}
}

#define	TRUE	1
#define	FALSE	0

static first = TRUE;

load_cart(fildes, num)
int fildes;
char num;
{
	int ret;

	errv ("tcio(1022): loading tape %s; please wait ...\n",fname);
	if (ioctl(fildes, CIOC_LOAD, &num) < 0)	{
		if (first)	{
			first = FALSE;
			unload_tape(fildes);
			if (ioctl(fildes, CIOC_LOAD, &num) >= 0)
				goto okay;
			}
		err("tcio(1023): cannot load cartridge %d; errno %d\n", num, errno);
		exit(errno);
		}
okay:
	close(fildes);
	fildes = open(fname,O_RDWR); 
	return(num);
}
