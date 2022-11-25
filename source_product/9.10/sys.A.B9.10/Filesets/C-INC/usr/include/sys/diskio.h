/*
 * @(#)diskio.h: $Revision: 1.5.83.4 $ $Date: 93/09/17 18:25:14 $
 * $Locker:  $
 */

/*
 *  $Header: diskio.h,v 1.5.83.4 93/09/17 18:25:14 kcs Exp $
 */

#ifndef _DISKIO_INCLUDED
#define _DISKIO_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

typedef struct {
	long	lba;  /* capacity in DEV_BSIZE blocks */
} capacity_type;

/*
 * Define the interface types used in describe structure.
 * Interface types 1000-1999 have been reserved for strategic alliances
 */

#define	UNKNOWN_INTF -1  /* Interface type unknown */
#define CS80_SS_INTF 00  /* HPIB disc0/1 Single Unit Single Port Controller */
#define CS80_MS_INTF 01  /* HPIB disc0/1 Multiple Unit Single Port Controller */
#define CS80_SM_INTF 02  /* HPIB disc0/1 Single Unit Multiple Port Controller */
#define CS80_MM_INTF 03  /* HPIB disc0/1 Multiple Unit Multiple Port Controller */
#define SS80_SS_INTF 04  /* HPIB SS80 Single Unit Single Port Controller */
#define SS80_MS_INTF 05  /* HPIB SS80 Multiple Unit Single Port Controller */
#define SS80_SM_INTF 06  /* HPIB SS80 Single Unit Multiple Port Controller */
#define SS80_MM_INTF 07  /* HPIB SS80 Multiple Unit Multiple Port Controller */
#define FLEX_SS_INTF 10  /* HPFL disc2/3 Single Unit Single Port Controller */
#define FLEX_MS_INTF 11  /* HPFL disc2/3 Multiple Unit Single Port Controller */
#define FLEX_SM_INTF 12  /* HPFL disc2/3 Single Unit Multiple Port Controller */
#define FLEX_PB_INTF 13  /* HPFL disc2/3 Multi Port P-Bus Only Controller */
#define FLEX_MM_INTF 14  /* HPFL disc2/3 Multi Unit Multiple Port Controller */

#define SCSI_INTF    20     /* SCSI interface */

/* device type defines */
#define	UNKNOWN_DEV_TYPE	-1  /* Device type unknown */
#define	DISK_DEV_TYPE		0   /* Disk device */
#define	CTD_DEV_TYPE		1   /* Cartridge tape device */
#define	WORM_DEV_TYPE		2   /* Write once read many optical device */
#define	MO_DEV_TYPE		3   /* Magneto Optical device */
#define	CDROM_DEV_TYPE		4   /* CDROM device */

typedef struct {
	char		model_num[16]; /* Model number in ascii */
	int		intf_type;     /* Interface type (CS80, SCSI, FLEX) */
	unsigned int	maxsva;	       /* Max single vector addr logical blks */
	unsigned int	lgblksz;       /* Logical Block Size */
	int		dev_type;      /* device type */
	unsigned int	flags;         /* Miscellaneous device flags */
} disk_describe_type;

#define WRITE_PROTECT_FLAG	0x1    /* Indicates WP tab set on MO/WORM media */

/*
 * Generic disk ioctls 
 */

#define DIOC_EXCLUSIVE	_IOW('D',101,int)
#define DIOC_CAPACITY	_IOR('D',102, capacity_type)
#define DIOC_DESCRIBE	_IOR('D',103, disk_describe_type)
#define DIOC_RSTCLR	_IOW('D',104, int)
#define DIOC_FORMAT     _IOW('D',105, int)

#endif /* _DISKIO_INCLUDED */
