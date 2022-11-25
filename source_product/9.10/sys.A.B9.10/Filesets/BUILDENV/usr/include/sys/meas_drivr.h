/*
 * @(#)meas_drivr.h: $Revision: 1.10.83.4 $ $Date: 93/09/17 18:29:23 $
 * $Locker:  $
 * 
 */

struct ms_nlist {
	char name[40];
	caddr_t address;
};

#define DEVMEAS_DRIVR	"/dev/meas_drivr"


/* ioctls for the ktest driver */
#define MS_NEWBUF	_IOWR('k', 8, int)
#define MS_FLUSHBUF	_IOWR('k', 9, int)
#define MS_MEAS		_IOWR('k', 10, int)
#define MS_TURN_ON	_IOWR('k', 11, int)
#define MS_TURN_OFF	_IOWR('k', 12, int)
#define MS_TURNED_ON	_IOWR('k', 13, int)
#define MS_NLIST	_IOWR('k', 14, struct ms_nlist)
