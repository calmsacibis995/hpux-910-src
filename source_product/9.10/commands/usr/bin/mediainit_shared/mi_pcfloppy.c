/* HPUX_ID: @(#) $Revision: 70.1 $  */
/*
 * PC Floppy mediainit
 */

/*
 * include files
 */
#ifdef _WSIO
#include <sys/sysmacros.h>
#else
#define _WSIO
#include <sys/sysmacros.h>
#undef _WSIO
#endif /* _WSIO */
#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/floppy.h> 
#include <sys/diskio.h> 
#include <unistd.h>

/*
 * globals from the parser
 */
extern int verbose;
extern int recertify;
extern int guru;
extern int fmt_optn;
extern int interleave;
extern int debug;
extern int blkno;
extern int verify_pass;
extern int maj;
extern int minr;
extern int fd;
extern int unit;
extern int volume;
extern int iotype;
extern char *name;

/*
 * other globals
 */
extern int errno;

/*
 * PC Floppy driver ioctl interface (s700)
 */

mi_pcfloppy()
{
	int flag=1, ret;
        disk_describe_type describe;
	struct floppy_info info;
	struct floppy_format_media fmt;

	/* Make sure that we have exclusive access to the device */
	verb("locking PC floppy device");
	if ((ret=ioctl(fd, DIOC_EXCLUSIVE, &flag))<0)	{
		perror("scsi_mi: Unable to set EXCLUSIVE mode");
		exit(1);
		}

	/* Make sure we have a floppy disk */
	if ((ret = ioctl(fd, FLOPPY_GET_INFO, &info))<0)	{
		perror("floppy_info: ioctl failed");
		exit(1);
		}

	verb("initializing media");
	fmt.fmt_option = fmt_optn;
	fmt.interleave = interleave;
	if ((ret = ioctl(fd, FLOPPY_FORMAT_MEDIA, &fmt))<0)	{
		err(errno, "initialize media command failed");
		exit(1);
		}

	exit(0);
}
