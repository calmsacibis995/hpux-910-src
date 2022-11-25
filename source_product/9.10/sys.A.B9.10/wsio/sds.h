/* @(#) $Revision: 1.1.83.3 $ */

#ifndef _SYS_SDS_INCLUDED
#define _SYS_SDS_INCLUDED

#ifdef _KERNEL_BUILD
#   include "../h/dirent.h"
#   include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#   include <sys/dirent.h>
#   include <sys/types.h>
#endif /* _KERNEL_BUILD */

#define SDS_MAXNAME	16
#define SDS_MAXDEVS	 8
#define SDS_MAXSECTS	 8	     /* for compatability with BSD */
#define SDS_MAXINFO	( 1 * 1024)  /* max size of SDS info */

/*
 * Error codes used internally by SDS routines
 */
#define SDS_OK		 0
#define SDS_OPENFAILED	 1
#define SDS_BADPRIMARY   2
#define SDS_DUPMEMBER    3
#define SDS_NOTMEMBER	 4
#define SDS_NOLIF	 5
#define SDS_NOTARRAY     6
#define SDS_NOTPRIMARY   7

struct sds_section {
    u_long start;	/* position from beginning of each disk */
    u_long stripesize;  /* size of each stripe */
    u_long size;	/* total size */
};

typedef struct sds_section sds_section_t;

struct sds_info
{
    char label[SDS_MAXNAME+1];           /* label of this array */
    char type[SDS_MAXNAME+1];            /* type of this array */
    u_long unique;			 /* "unique" id of this array */
    int which_disk;			 /* which disk we are */
    u_short ndevs;	                 /* number of disks in array */
    dev_t devs[SDS_MAXDEVS];             /* devices in this array */
    sds_section_t section[SDS_MAXSECTS]; /* each volume in the array */
};

typedef struct sds_info sds_info_t;

/*
 * Various keywords returned by sds_keyword()
 */
#define SDS_KW_ERROR		0
#define SDS_KW_DISK	        1
#define SDS_KW_LABEL		2
#define SDS_KW_TYPE		3
#define SDS_KW_UNIQUE		4
#define SDS_KW_DEVICE		5
#define SDS_KW_SECTION		6
#define SDS_KW_START		7
#define SDS_KW_STRIPE		8
#define SDS_KW_SIZE		9
#define SDS_KW_EOF	       10

/*
 * CS80, SCSI, AUTOCHANGER and OPAL major numbers
 */
#define CS80_MAJ_BLK   0
#define CS80_MAJ_CHR   4
#define SCSI_MAJ_BLK   7
#define SCSI_MAJ_CHR  47
#define AC_MAJ_BLK    10
#define AC_MAJ_CHR    55
#define OPAL_MAJ_BLK  14
#define OPAL_MAJ_CHR 104

/*
 * The name of the SDS_INFO file, as stored in the LIF volume.
 * Must be blank padded to 10 characters long.
 */
#define SDS_INFO	"SDS_INFO  "

/**************************** union Device ***********************************/
/*                                                                           */
/* This union defines the fields of an SDS device number.                    */
/*                                                                           */
/* field.majorNumber     - driver number (7 block, 47 char)                  */
/* field.selectCode      - interface select code (0x20 core, 0x41-0x44 EISA) */
/* field.functionNumber  - interface function number (0 EISA, 1 core)        */
/* field.busAddress      - SCSI target bus address (0 thru 7)                */
/* field.logicalUnit     - SCSI target logical unit number (0 thru 7)        */
/* field.partition       - SDS partition number (1 thru 8)                   */
/* dev                   - dev_t                                             */
/*                                                                           */
/*****************************************************************************/

union Device {
  struct {
    unsigned majorNumber:8;
    unsigned selectCode:8;
    unsigned functionNumber:4;
    unsigned busAddress:4;
    unsigned logicalUnit:4;
    unsigned partition:4;
  } field;
  dev_t dev;
};


/*************************** struct Link *************************************/
/*                                                                           */
/* This structure defines the header for all sds linked lists.  Any structs  */
/* to be linked must have this as their first component.  Note that it is    */
/* a subset of a "struct buf", so bufs can be linked using the SDS linked    */
/* list routines.                                                            */
/*                                                                           */
/*****************************************************************************/

struct Link {
  int unused[3];
  struct Link *next;
  struct Link *prev;
};


/***************************** struct Volume *********************************/
/*                                                                           */
/* This structure contains all of the information needed to access an SDS    */
/* array volume.  It describes the physical and logical characteristics of   */
/* that volume.                                                              */
/*                                                                           */
/* link           - link pointers                                            */
/* device         - SDS array volume major/minor number                      */
/* label          - SDS array volume name (array-unique)                     */
/* type           - SDS array volume type (configuration-unique)             */
/* flags.locked   - DIOC_EXCLUSIVE open (boolean)                            */
/* flags.busy     - sds_config in progress (boolean)                         */
/* deviceCount    - number of physical devices in SDS array                  */
/* devices        - physical device major/minor numbers                      */
/* openCount      - SDS array volume open count                              */
/* blockSize      - device physical block size (in bytes)                    */
/* log2BlockSize  - log2 of device physical block size                       */
/* physOffset     - physical device offset of volume (in bytes)              */
/* volumeSize     - size of volume (in device blocks)                  	     */
/* stripeSize     - stripe size (in bytes)                                   */
/* log2StripeSize - log2 of stripe size                                      */
/* bdevsw         - pointer to physical device driver bdevsw entry           */
/* cdevsw         - pointer to physical device driver cdevsw entry           */
/* freeBufs       - preallocated bufs for use by SDS                         */
/* freeBufCount   - number of preallocated bufs                              */
/* freeBufMax     - maximum number of preallocated bufs we might ever need   */
/* waitingBufs    - primary bufs waiting to be processed                     */
/*                                                                           */
/*****************************************************************************/

struct Volume {
  struct Link link;
  union Device device;
  char label[SDS_MAXNAME+1];
  char type[SDS_MAXNAME+1];
  struct Flags {
    unsigned locked:1;
    unsigned busy:1;
  } flags;
  unsigned deviceCount;
  union Device devices[SDS_MAXDEVS];
  unsigned openCount;
  unsigned blockSize;
  unsigned log2BlockSize;
  unsigned physOffset;
  unsigned volumeSize;
  unsigned stripeSize;
  unsigned log2StripeSize;
  struct bdevsw *bdevsw;
  struct cdevsw *cdevsw;
  struct Link freeBufs;
  unsigned freeBufCount;
  unsigned freeBufMax;
  struct Link waitingBufs;
};

#endif /* _SYS_SDS_INCLUDED */
