/* -*-C-*-
********************************************************************************
*
* File:         vdi.h
* RCS:          $Header: vdi.h,v 70.4 94/10/12 15:36:31 hmgr Exp $
* Description:  This is the include file for the virtual device interface
* Author:       Dan Matheson, CSB R&D
* Created:      Mon Nov 19 14:22:51 1990
* Modified:     Mon Jan  7 15:44:47 1991 (Dan Matheson) danm@kheldar
* Language:     elec-C
* Package:      N/A
* Status:       Experimental (Do Not Distribute)
*
* (c) Copyright 1990, Hewlett-Packard Company, all rights reserved.
*
********************************************************************************
*/

/*

  The virtual device interface is a device independent interface to all 
  the backup hardware.  The idea is to hide all device specific stuff.
  This will let the code be leaveraged between fbackup/frecover and 
  hsbackup/hsrestore.  This was done as part of the addition of 2 new
  device types to the ones allowed by fbackup: DAT with fast search and
  raw magneto-optical disks.

  For fbackup and frecover the vdi code is in the module tape.c.  This
  was done to keep changes in makefiles and shared source files to a
  minimum.

*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef HSB_KLUDGE_7_X
#include <sio/autoch.h>
#include <sys/diskio.h>
#endif /* HSB_KLUDGE_7_X */

#include <sys/cs80.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/scsi.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>

#if !defined(SIOC_CAPACITY)
#define SIOC_CAPACITY                   _IOR('S', 3, struct capacity)
#endif  

#ifdef HSB_KLUDGE_7_X
#ifdef __hp9000s300
#define GMT_EOF(x)              ((x) & 0x80000000)
#define GMT_BOT(x)              ((x) & 0x40000000)
#define GMT_EOT(x)              ((x) & 0x20000000)
#define GMT_WR_PROT(x)          ((x) & 0x04000000)
#define GMT_ONLINE(x)           ((x) & 0x01000000)
#define GMT_D_6250(x)           ((x) & 0x00800000)
#define GMT_D_1600(x)           ((x) & 0x00400000)
#define GMT_D_800(x)            ((x) & 0x00200000)
#define GMT_DR_OPEN(x)          ((x) & 0x00040000)
#define GMT_IM_REP_EN(x)        ((x) & 0x00010000)
#endif  /* __hp9000s300 */
#endif /* HSB_KLUDGE_7_X */

#define FALSE 0
#define TRUE 1

#define CLOSED -1

#define ATTREW	  1
#define UCBNOREW  2
#define ILLEGALMT 3
#define UCBREW	        4
#define ATTNOREW	5
#define PROTECT 0600			/* protection mode for created files */

/* vdi external variables */

int  vdi_errno;             /* system errno that happened with vdi call */
char vdi_name[MAXPATHLEN];  /* vdi device name */


/* output file types */

#define VDI_UNKNOWN 100
#define VDI_REGFILE 101
#define VDI_MAGTAPE 102
#define VDI_DAT     103
#define VDI_DAT_FS  104
#define VDI_MO      105
#define VDI_DISK    106
#define VDI_STDOUT  107
#define VDI_REMOTE  108

/* vdi error numbers */

#define VDIERR_NONE     0    /* no error value */
#define VDIERR_ID      -1    /* could not identify device */
#define VDIERR_NOD     -2    /* no device there */
#define VDIERR_NOM     -3    /* no media there */
#define VDIERR_NOWR    -4    /* not writable */
#define VDIERR_READ    -5    /* read error */
#define VDIERR_WRT     -6    /* write error */
#define VDIERR_OP      -7    /* undefined operation */
#define VDIERR_ACC     -8    /* could not access device */
#define VDIERR_TYPE    -9    /* illegal device type */
#define VDIERR_REM    -10    /* remote device not supported */
#define VDIERR_IN     -11    /* vdi internal error */

/* vdi identify call information */

  /* nothing special here */

/* vdi open call information */

  /* use open flags */

/* vdi close call information */

  /* use close flags */

/* vdi read call information */

  /* use read flags */

/* vdi write call information */

  /* use write flags */

/* vdi get attributes call information */

struct vdi_gatt {
  long wrt_protect;  /* boolean write protected */
  long media;          /* boolean media present */
  long on_line;        /* boolean on-line */
  long bom;            /* boolean at beginning of media */
  long eom;            /* boolean at end of media */
  long eod;            /* boolean at end of data */
  long tm1;            /* boolean at tape mark type 1 (EOF) */
  long tm2;            /* boolean at tape mark type 2 (Fast Search) */
  long im_report;      /* boolean immediate report mode on */
  long door_open;      /* boolean door open */
  long density;        /* current write density */
  long optimum_block;  /* optimum block size for transfer */
  long wait_time;      /* waiting time for next request */
  long hog_time;       /* minimum time surface held in device */
  long queue_size;     /* number of requests in queue */
  long queue_mix;      /* number of different surfaces in queue */
};

#define VDIGAT_NA    -1    /* not applicable value */
#define VDIGAT_NO     0    /* no answer to binary status */
#define VDIGAT_YES    1    /* yes answer to binary status */

/* get attribute operations */

#define VDIGAT_ISWP    0x00000001    /* is media write protected */
#define VDIGAT_ISMP    0x00000002    /* is media present */
#define VDIGAT_ISOL    0x00000004    /* is device on-line */
#define VDIGAT_ISBOM   0x00000008    /* at beginning of media */
#define VDIGAT_ISEOM   0x00000010    /* at end of media */
#define VDIGAT_ISTM1   0x00000020    /* at tape mark 1 (EOF) */
#define VDIGAT_ISTM2   0x00000040    /* at tape mark 2 (Fast Search) */
#define VDIGAT_ISIR    0x00000080    /* is immediate report mode on */
#define VDIGAT_ISDO    0x00000100    /* is door open */
#define VDIGAT_ISDEN   0x00000200    /* what is write density */
#define VDIGAT_ISOB    0x00000400    /* what is optimal block size */
#define VDIGAT_ISQC    0x00000800    /* what are queue constants */
#define VDIGAT_ISQS    0x00001000    /* what are queue statistics */
#define VDIGAT_ISEOD   0x00002000    /* at end of data? */


/* vdi set attributes call information */

struct vdi_satt {
  long wait_time;      /* waiting time for next request */
  long hog_time;       /* minimum time surface held in device */
};

/* set attributes operations */

#define VDISAT_CON     1    /* compression on */
#define VDISAT_COFF    2    /* compression off */
#define VDISAT_QCONS   3    /* set queue constants */
#define VDISAT_800     4    /* set writing density to 800 bpi */
#define VDISAT_1600    5    /* set writing density to 1600 bpi */
#define VDISAT_6250    6    /* set writing density to 6250 bpi */


/* device operations */

#define VDIOP_NOP    42    /* no operation sets status */
#define VDIOP_SOFFL   1    /* rewind and put device off-line */
#define VDIOP_WTM1    2    /* write an EOF */
#define VDIOP_FSF     3    /* forward space file */
#define VDIOP_BSF     4    /* backward space file */
#define VDIOP_FSR     5    /* forward space record */
#define VDIOP_BSR     6    /* backward space record */
#define VDIOP_REW     7    /* rewind */
#define VDIOP_EOD     8    /* seek to EOD */
#define VDIOP_WTM2    9    /* write save setmarks */
#define VDIOP_FSS    10    /* forward space save setmarks */
#define VDIOP_BSS    11    /* backward space save setmarks */


#define GMT_8mm_FORMAT(x)       ((x) & 0x000000ff)
#define FORMAT8mm8500c          DEN_8MM_8500c
#define DEN_8MM_8500c      0x0000008c /* 8MM 8500 Compressed Format */
