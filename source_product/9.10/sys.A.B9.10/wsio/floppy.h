/*
 * @(#)floppy.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 20:29:02 $
 * $Locker:  $
 */

#ifndef _SYS_FLOPPY_INCLUDED
#define _SYS_FLOPPY_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */


/* structure for FLOPPY_GET_STATUS ioctl */
struct floppy_info {
    unsigned media;
    unsigned status;
    unsigned valid;
};

/* values for media (media diameter in millimeters) */
#define FLOPPY_UNKNOWN	  0
#define FLOPPY_3INCH	 90
#define FLOPPY_5INCH	130
#define FLOPPY_8INCH	200

/* macros for checking status and valid */
#define FLOPPY_NO_MEDIA(x)	((x) & 0x00000001)
#define FLOPPY_BLANK_MEDIA(x)	((x) & 0x00000002)
#define FLOPPY_WRITE_PROT(x)	((x) & 0x00000004)
#define FLOPPY_MEDIA_CHANGED(x)	((x) & 0x00000008)
#define FLOPPY_HIGH_DENSITY(x)	((x) & 0x00000010)


/* structure for FLOPPY_GET_GEOMETRY and FLOPPY_SET_GEOMETRY ioctls */
struct floppy_geometry {
    unsigned heads;
    unsigned tracks;
    unsigned sectors;
    unsigned sector_size;
    unsigned transfer_rate;
    unsigned track_density;
    unsigned data_encoding;
};

/* values for data encoding */
#define FLOPPY_UNKNOWN_M     0		/* Unknown Modulation */
#define FLOPPY_FM	     1		/* Frequency Modulation */
#define FLOPPY_MFM	     2		/* Modified Frequency Modulation */


/* structure for FLOPPY_FORMAT ioctl */
struct floppy_format {
    unsigned head;
    unsigned track;
    unsigned interleave;
};


/* ioctls for flexible "floppy" disk devices */
#define	FLOPPY_GET_INFO		_IOR('F', 1, struct floppy_info)
#define	FLOPPY_GET_GEOMETRY	_IOR('F', 2, struct floppy_geometry)
#define	FLOPPY_SET_GEOMETRY	_IOW('F', 3, struct floppy_geometry)
#define	FLOPPY_FORMAT_TRACK	_IOW('F', 4, struct floppy_format)


#endif /* _SYS_FLOPPY_INCLUDED */
