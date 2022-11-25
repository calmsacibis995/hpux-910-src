/* HPUX_ID: @(#) $Revision: 66.1 $  */
/*
 * AC (autochanger) mediainit
 */


/*
 * include files
 */
#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sio/autoch.h>


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
 * AC (autochanger) driver ioctl interface
 */

mi_acinit()
{
	int ret;

	verb("initializing media");
	if ((ret=ioctl(fd, ACIOC_FORMAT, &interleave))<0)  {
		err(errno, "initialize media command failed");
		exit(1);
		}

}
