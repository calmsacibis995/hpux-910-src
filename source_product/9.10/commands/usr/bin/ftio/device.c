/* HPUX_ID: @(#) $Revision: 64.1 $  */
/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : device.c 
 *	Purpose ............... : Device and system specific calls. 
 *	Author ................ : David Williams. 
 *
 *	Description:
 *
 *
 *-----------------------------------------------------------------------------
 */

#include "ftio.h"
#include "define.h"	
/* #include <sys/types.h> */
#include <sys/mtio.h>	
#include <errno.h>	

#ifdef	hp9000s800
#include <sys/ioctl.h>
#endif 	hp9000s800

#ifndef	MT_EOF
#define	MT_EOF	0x80000000	/* end of file */	
#endif
#ifndef	MT_EOT
#define	MT_EOT	0x20000000	/* end of tape foil */	
#endif
#ifndef	MT_WR_PROT
#define	MT_WR_PROT 0x04000000	/* write protected */
#endif


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : test_dev_type()
 *	Purpose ............... : See what kind of device we are connected to. 
 *
 *	Description:
 *
 *	Returns:
 *
 *	device type code. 
 *
 */

test_dev_type(fd)
int	fd;
{
	struct	stat	stats;

	long	get_tape_stat();

	if (rmt_fstat(fd, &stats) == -1)
		return -1;

	if ((stats.st_mode & S_IFMT) == S_IFREG)
	{
		Dev_type = REGULAR;
		Tape_headers = 0;
		return REGULAR;
	}

	if (get_tape_stat(fd, 0) == -1)
	{
		Dev_type = UNKNOWN;
		Tape_headers = 0;
		return UNKNOWN;
	}

	Dev_type = MAG_TAPE;

	return MAG_TAPE;
}


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : Test_eot()
 *	Purpose ............... : Tests if at EOT. 
 *
 *	Description:
 *
 *	Returns:
 *	
 *	-1 	if fail.
 *	0	if not nothing (not at end of tape).
 *	1	if EOT foil.
 *	2	if EOF mark.
 *
 */
test_eot(fd, retval)
int	fd;
int	retval; 
{
	long	stat_ret;

	long	get_tape_stat();

	if (Dev_type == REGULAR)
	{
		if (retval == -1)
			return ABORT;
		return FINISH;
	}

	if ((stat_ret = get_tape_stat(fd, 0)) == -1)
		return NEXT;  

	if (stat_ret & MT_EOT)
		return NEXT;

	if (stat_ret & MT_EOF)
		return FINISH;

	return NEXT;
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : Write_eof()
 *	Purpose ............... : Calls ioctl(2) to write an EOF mark.
 *
 *	Description:
 *
 *	Returns:
 *		Ioctl(2) return status.
 *
 */
write_eof(fd)
int	fd;
{
	struct	mtop	top;

	top.mt_op = MTWEOF;
	top.mt_count = 1;

	return(rmt_ioctl(fd, MTIOCTOP, &top));
}

read_eof(fd)
int	fd;
{
	char	buf[1];

	return(rmt_read(fd, buf, 1));
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : wr_protect()
 *	Purpose ............... : Test if write protected. 
 *
 *	Description:
 *
 *	Returns:
 *
 *	-1 for fail boolean value of state otherwise. 
 *
 */
wr_protect(fd)
int	fd;
{
	long	stat_ret;

	long	get_tape_stat();

	if ((stat_ret = get_tape_stat(fd, 0)) == -1)
		return -1;

	return stat_ret & MT_WR_PROT;
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : get_tape_stat()
 *	Purpose ............... : Gets status of tape drive. 
 *
 *	Description:
 *		Get_tape_stat() uses a ioctl() call to stat the drive
 * 		defined by dev_fd. If test is non-zero, then it's value
 *		is logically ANDed with the stat structure, and the
 *		boolean result returned. If test is zero, the entire
 *		stat bit mask is returned.
 *
 *	Returns:
 *		see description.
 */

long
get_tape_stat(dev_fd, test)
int	dev_fd;
long	test;
{
	struct mtget get;

	/*
	 * call ioctl and return stats in get
	 * note: return NULL if failed.
		ftio_mesg(FM_IOCTL);
	 */
	if (rmt_ioctl(dev_fd, MTIOCGET, &get) == -1)
		return -1;

#ifdef hp9000s500
	if (test)
		return(get.mt_dsreg & test);
	else
		return(get.mt_dsreg);
#else
	if (test)
		return(get.mt_gstat & test);
	else
		return(get.mt_gstat);
#endif
}

test_write_err(fd)
int	fd;
{
#ifdef	hp9000s500
	extern	int	errinfo;
#else
	extern	int	errno;
#endif	hp9000s500

#ifdef	lint
	printf("%d", fd);
#endif	lint

	/*
	 *	A regular file should never have a write error.
	 */
	if (Dev_type == REGULAR)
		return ABORT;

	/*
	 *	If we don't know about this device, then
	 *	go for the next tape/medium.
	 */
	if (Dev_type != MAG_TAPE)
		return NEXT;

	/*
	 *	Mag_tape...
	 */
#ifdef	hp9000s500
	if (errinfo != 128)
#else
	if (errno != ENOSPC)
#endif	hp9000s500
		return RESTART;

	return NEXT;
}
