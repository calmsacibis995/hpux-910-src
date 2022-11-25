/* @(#) $Revision: 27.2 $ */    

/* internal MINIFLOPPY mediainit interface */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mf.h>

/* globals from the parser */
extern int verbose;
extern int recertify;
extern int guru;
extern int fmt_optn;
extern int interleave;
extern int fd;
extern int unit;
extern int volume;

/* other globals */
extern int errno;

/* MINIFLOPPY driver ioctl interface */
void mi_mfinit()
{
	struct MF_init_track mfinits;
	register error;

/* validate the supplied interleave factor or come up with a default */
    	if (interleave < 0)
		err(EINVAL, "interleave must be a positive number");
	if (interleave == 0)
		interleave = 1;
    	if (interleave > 15)
		err(EINVAL, "maximum interleave for this device is 15");

/* initialize the media */
    	verb("initializing media at interleave = %d", interleave);

/* temp method */
	mfinits.phy_track_num = interleave;
	mfinits.crt_ptr = (guru) ? (char *)0x513000 : (char *)0;

/* now go try to do the initialize */
	if (ioctl(fd, MFINITS, &mfinits)) {

		switch (errno) {
		case EBUSY:
			err(errno, "can't lock volume");
		case EINVAL:
		default:
			err(errno, "initialize media command failed");
		}
	}
/*  if no error, then check if error from the asmb driver */
	switch (error = mfinits.error) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
/*  report number of spared tracks if successful */
		verb("number of spared tracks is = %d", error);
		return;
/*  error is from the asmb driver -- report the detailed error */
	case 1066:
		err(error, "track zero is defective -- cannot be spared");
	case 2066:
		err(error, "need more than 4 spare tracks");
	case 3066:
	case 4066:
	case 5066:
	case 6066:
	case 7066:
	case 7090:
		err(error, "system too busy or media missing -- fix and try again");
	case 8080:
		err(error, "media changed during initialize");
	case 2080:
		err(error, "media missing index hole");
	case 1082:
		err(error, "drive missing");
	case 2083:
		err(error, "media write protected");
	default:
		err(errno, "initialize media command failed");
	}
}
