/* @(#) $Revision: 70.1 $ */

/***************************************************************************
****************************************************************************

	tape.c

    This file contains the functions which manipulate, control, and query
    output files.  Normally we expect these files to be magnetic tape drives,
    however, they may also be regular files.  Most of the functions in this
    file are not necessary for regular files; they are designed to handle
    tape drives.

    As of 8.0 the types of devices supported include DAT with or without
    fast search capability and MO disks (and by default other disks as well
    since they should behave as MO disks).

    All of the device specific things will be hidden behind a virtual
    device interface.

****************************************************************************
***************************************************************************/

#include "head.h"
#include <fcntl.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef NLS
#define NL_SETN 3	/* message set number */
extern nl_catd nlmsg_fd;
#endif NLS

void writelabel(), writevolhdr(), writeidx();

#define UCB_800    0x100000
#define NOREW_800  0x080000

#define UCB_300	        0x2
#define NOREW_300	0x1


PADTYPE	*pad;
RECTYPE *rec;


extern int  n_outfiles,
	   trecnum,
	   volfirstrec;

extern int machine_type;         /* machine type where the tape device is, rmt.c
			     can overwrite */

char *host;		/* remote host name */


/***************************************************************************
    This function determines the "type" of raw magtape device.  Getmttype
    in interested in two flags: the AT&T/UCB flag and the rewind-on-close
    flag.  There are presently two legal types of raw drives (block devices
    aren't supported): AT&T mode with rewind-on-close set, and UCB mode
    without rewind-on-close.  The other two combinations are illegal.
    There are three return values for these three conditions.

    danm addition: The checks need to be made with respect to the
    type of system the mag tape device is on.  This fact is recorded
    in the variable machine_type.
***************************************************************************/
int
getmttype(statbuf)
struct stat *statbuf;
{
  int prob = 0;
  
  if (machine_type == 800) {
    if (!(statbuf->st_rdev & UCB_800) && !(statbuf->st_rdev & NOREW_800))
      return(ATTREW);
    else if ((statbuf->st_rdev & UCB_800) && (statbuf->st_rdev & NOREW_800))
      return(UCBNOREW);
    else {
      prob = 1;
    }
  }
  
  
  if ((machine_type == 300) || (machine_type == 700)) {
    if ((statbuf->st_rdev & UCB_300) && (statbuf->st_rdev & NOREW_300))
        return(UCBNOREW);
    else if ((statbuf->st_rdev & UCB_300) && !(statbuf->st_rdev & NOREW_300))
        return(UCBREW);
    else if (!(statbuf->st_rdev & UCB_300) && !(statbuf->st_rdev & NOREW_300))
        return(ATTREW);
    else if (!(statbuf->st_rdev & UCB_300) && (statbuf->st_rdev & NOREW_300))
        return(ATTNOREW);
    else {
      prob = 1;
    }
  }
  
  if (prob == 1) {
    return(ILLEGALMT);
  }
}






extern char *recbuf;

/***************************************************************************
    This function checks to see if a tape volume is "OK".  It tries to read
    (a label and) a volume header.  If this volume was last used for something
    other than fbackup, we know nothing of the history of the volume, and it
    is treated as being new (the number of uses is set to 1).  If it was last
    used by fbackup, it is first checked to see if it is a volume of this
    session.  If it is, the volume is rejected.  Next, n_uses (th number of
    times this volume has been used by fbackup) is extracted and incremented.
    If the number is too high, newtapeok asks if you want to use it anyway.
    If not, the new tape is rejected.  Otherwise, the tape is accepted.
***************************************************************************/
int
newtapeok(outfiletype, fd, n_uses)
     int outfiletype;
     int fd;
     int *n_uses;
{
  VHDRTYPE volhdr;
  int n=0;
  int retval=TRUE;
  int res;
  int res1;
  int res2;
  int res3;

  switch (outfiletype)	
    {	       
      case VDI_MAGTAPE:
        res = ((vdi_read(outfiletype, fd, recbuf, (unsigned)sizeof(LABELTYPE)+1) == sizeof(LABELTYPE))
	       && ((vdi_read(outfiletype, fd, recbuf, (unsigned) 10) == 0))
	       && (vdi_read(outfiletype, fd, &volhdr, (unsigned)sizeof(VHDRTYPE)+1) == sizeof(VHDRTYPE)));
        break;	  
      case VDI_DAT:
      case VDI_DAT_FS:
        res1 = (vdi_read(outfiletype, fd, recbuf, (unsigned)sizeof(LABELTYPE)) == sizeof(LABELTYPE));
	res2 = (vdi_read(outfiletype, fd, recbuf, (unsigned) 10) == 0);
	res3 = (vdi_read(outfiletype, fd, &volhdr, (unsigned)sizeof(VHDRTYPE)) == sizeof(VHDRTYPE));
	res = res1 && res2 && res3;
#ifdef DEBUG_NEWTAPE
fprintf(stderr, "NEWTAPEOK : DAT res1 = %d, label = %s\n", res1, recbuf);
fprintf(stderr, "NEWTAPEOK : DAT res2 = %d, EOF = %s\n", res2, recbuf);
fprintf(stderr, "NEWTAPEOK : DAT res3 = %d, volheader = %s\n", res3, volhdr.magic);
#endif 
        break;	  
      case VDI_MO:
        res1 = (vdi_read(outfiletype, fd, recbuf, (unsigned)sizeof(LABELTYPE)) == sizeof(LABELTYPE));
	res2 = ((n = vdi_read(outfiletype, fd, &volhdr, (unsigned)sizeof(VHDRTYPE))) == sizeof(VHDRTYPE));
	res = res1 && res2;
#ifdef DEBUG_NEWTAPE
fprintf(stderr, "NEWTAPEOK : MO res = %d, label = %s\n", res1, recbuf);
fprintf(stderr, "NEWTAPEOK : MO vol read n = %d\n", n);
n = sizeof(VHDRTYPE);
fprintf(stderr, "NEWTAPEOK : MO sizeof VDHRTYPE = %d\n", n);
fprintf(stderr, "NEWTAPEOK : MO vol read vdi_errno = %d\n", vdi_errno);
fprintf(stderr, "NEWTAPEOK : MO res = %d, volheader = %s\n", res2, volhdr.magic);
#endif 
        break;	  
      case VDI_DISK:
        res1 = (vdi_read(outfiletype, fd, recbuf, (unsigned)sizeof(LABELTYPE)) == sizeof(LABELTYPE));
	res2 = (vdi_read(outfiletype, fd, volhdr, (unsigned)sizeof(VHDRTYPE)) == sizeof(VHDRTYPE));
	res = res1 && res2;
        break;	  
    } /* switch */	
  
  if (res && (strcmp(volhdr.magic, VOLMAGIC) == 0)) {
    
    /* got a label and vol header */
    
    if ((atoi(volhdr.backupid.ppid) == pad->pid) &&
	(atoi(volhdr.backupid.time) == pad->begtime)) {
      n = atoi(volhdr.volno);
      msg((catgets(nlmsg_fd,NL_SETN,202, "fbackup(3202): this is volume %d OF THIS SESSION!\n	rejecting this volume\n")), n);
      retval = FALSE;
    } else {
      *n_uses = atoi(volhdr.mediause);
      msg((catgets(nlmsg_fd,NL_SETN,203, "fbackup(3203): volume %d has been used %d time(s)\n")), pad->vol, *n_uses);
      if ((outfiletype == VDI_MAGTAPE) || (outfiletype == VDI_DAT) || (outfiletype == VDI_DAT_FS)) {
	if ((*n_uses >= pad->maxvoluses) &&
	    (!query((catgets(nlmsg_fd,NL_SETN,204, "fbackup(3204): volume used more than maximum, do you want to use this volume anyway?\n")))))
	  retval = FALSE;
      }
    } 
  } else {		/* didn't get a label and/or a vol header */
    msg((catgets(nlmsg_fd,NL_SETN,205, "fbackup(3205): unable to read a volume header\n")));
    *n_uses = 0;
  }
  (*n_uses)++;
  
  switch (outfiletype)	
      {	
      case VDI_MAGTAPE:
	vdi_ops(outfiletype, fd, VDIOP_REW, 1);
	break;	
      case VDI_DAT:
      case VDI_DAT_FS:
/*
 * Indicate that we could not read the label on the DAT.
 */
	if (!res1)
	  *n_uses = -1;
	else
	vdi_ops(outfiletype, fd, VDIOP_REW, 1);
	break;	
      default:
	break;
      } /* switch */	
  return(retval);
}  /* end newtapeok */

